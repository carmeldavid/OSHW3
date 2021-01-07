#ifndef __GAMERUN_H
#define __GAMERUN_H

#include "../Part1/Headers.hpp"
#include "../Part1/PCQueue.hpp"
#include "Thread.hpp"


/*--------------------------------------------------------------------------------
								  Species colors
--------------------------------------------------------------------------------*/
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black - 7 */
#define RED     "\033[31m"      /* Red - 1*/
#define GREEN   "\033[32m"      /* Green - 2*/
#define YELLOW  "\033[33m"      /* Yellow - 3*/
#define BLUE    "\033[34m"      /* Blue - 4*/
#define MAGENTA "\033[35m"      /* Magenta - 5*/
#define CYAN    "\033[36m"      /* Cyan - 6*/


/*--------------------------------------------------------------------------------
								  Auxiliary Structures
--------------------------------------------------------------------------------*/
struct game_params {
	// All here are derived from ARGV, the program's input parameters. 
	uint n_gen;
	uint n_thread;
	string filename;
	bool interactive_on; 
	bool print_on; 
};
/*--------------------------------------------------------------------------------
									Class Declaration
--------------------------------------------------------------------------------*/



class Game {
public:

    class Task {
    public:
        Task(Game *g, unsigned int start, unsigned int end,unsigned int taskPhase) : game(g),phase(taskPhase) , startRow(start), endRow(end) {}
        ~Task() = default;
        void doTask() {

                auto gen_start = std::chrono::system_clock::now();
                if (phase == 1)
                    game->phase1(startRow, endRow);
                if (phase == 2)
                    game->phase2(startRow, endRow);

                pthread_mutex_lock(&game->lock); // Atomic counter and hist
                game->finished++;

                if(game->phaseFinish())
                    pthread_cond_signal(&game->fin);

                auto gen_end = std::chrono::system_clock::now();
                game->m_tile_hist.push_back(
                        (double) std::chrono::duration_cast<std::chrono::microseconds>(gen_end - gen_start).count());

            pthread_mutex_unlock(&game->lock);

        }
    private:
        Game* game;
        unsigned int phase;
        unsigned int startRow;
        unsigned int endRow;
    };



    Game(game_params p) : gen(0),g(p) {};
	~Game() = default;
	void run(); // Runs the game
	const vector<double> gen_hist() const {return m_gen_hist;}; // Returns the generation timing histogram
	const vector<double> tile_hist() const  {return m_tile_hist;}; // Returns the tile timing histogram
	uint thread_num() const {return m_thread_num;}; //Returns the effective number of running threads = min(thread_num, field_height)
    uint gen_num() const {return m_gen_num;};

    vector<vector<unsigned int>> livingNeighbors(unsigned int row,unsigned int col);
    void swapMatrices();
    bool phaseFinish() const {return finished == m_thread_num;};

    void phase1(unsigned int startLine,unsigned int endLine);
    void update1(unsigned int row,unsigned int col);
    void update2(unsigned int row,unsigned int col);
    void phase2(unsigned int startLine,unsigned int endLine);
    Task* popTask() {return tasksToThreads.pop();}
    pthread_mutex_t lock;
    pthread_mutex_t popLock;

    pthread_cond_t fin;
    unsigned int totPops; // Total Queue Pops

    unsigned int gen; // Curr Gemeration

protected: // All members here are protected, instead of private for testing purposes

	// See Game.cpp for details on these three functions
	void _init_game(); 
	void _step(uint curr_gen); 
	void _destroy_game(); 
	inline void print_board(const char* header);

	uint m_gen_num; 			 // The number of generations to run
	uint m_thread_num; 			 // Effective number of threads = min(thread_num, field_height)
	vector<double> m_tile_hist; 	 // Shared Timing history for tiles: First (2 * m_gen_num) cells are the calculation durations for tiles in generation 1 and so on. 
							   	 // Note: In your implementation, all m_thread_num threads must write to this structure. 
	vector<double> m_gen_hist;  	 // Timing history for generations: x=m_gen_hist[t] iff generation t was calculated in x microseconds
	vector<Thread*> m_threadpool; // A storage container for your threads. This acts as the threadpool. 

	bool interactive_on; // Controls interactive mode - that means, prints the board as an animation instead of a simple dump to STDOUT 
	bool print_on; // Allows the printing of the board. Turn this off when you are checking performance (Dry 3, last question)

	// TODO: Add in your variables and synchronization primitives  
    game_params g;

    PCQueue<Task*> tasksToThreads;
    unsigned int finished; // Threads Finished

    vector<vector<unsigned int>>* curr;
    vector<vector<unsigned int>>* next;
    unsigned int matrixSize[2]; //[Rows,Columns]


};


class Tile : public Thread{
public:
    Tile(uint threadId, Game *g) : Thread(threadId), game(g) {};
    void thread_workload() {
        Game::Task *t;

        while (1) {

            pthread_mutex_lock(&game->popLock);
            if(game->totPops == 2*game->thread_num()*game->gen_num()) {
                pthread_mutex_unlock(&game->popLock);
                return;
            }
            game->totPops++;
            pthread_mutex_unlock(&game->popLock);
            t = game->popTask();

            t->doTask();
            delete t;
            /*while(!game->phaseFinish())
                pthread_cond_wait(&game->fin,&game->lock);*/

        }
    }
private:
    Game* game;
};

#endif
