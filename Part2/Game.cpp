#define DEAD 0
#define SPECIES 7

#include "Game.hpp"
#include "utils.hpp"
#include <string>

static const char *colors[SPECIES] = {BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};
/*--------------------------------------------------------------------------------
								
--------------------------------------------------------------------------------*/
int min(int a, int b) {
    if (a < b)
        return a;
    return b;
}

void Game::run() {

	_init_game(); // Starts the threads and all other variables you need
    print_board("Initial Board");
	for (gen = 0; gen < m_gen_num; ++gen) {

        auto gen_start = std::chrono::system_clock::now();
		_step(gen); // Iterates a single generation

        auto gen_end = std::chrono::system_clock::now();
		m_gen_hist.push_back((double)std::chrono::duration_cast<std::chrono::microseconds>(gen_end - gen_start).count());
		print_board(nullptr);
	} // generation loop

    for (unsigned int i = 0; i < matrixSize[0]; i++)
    {
        for (unsigned int j = 0; j < matrixSize[1]; j++)
        {
            cout << (*curr)[i][j] << " ";
        }

        // Newline for new row
        cout << endl;
    }

    cout<< endl<< endl;
    print_board("Final Board");
	_destroy_game();
}




vector<vector<unsigned int>> Game::livingNeighbors(unsigned int row,unsigned int  col) {


    vector<vector<unsigned int>> v;
    unsigned up = 1,down = 1 ,left = 1 ,right = 1;
    if(!row) up = 0;
    if(!col) left = 0;
    if(row == matrixSize[0] - 1) down = 0;
    if(col == matrixSize[1] - 1) right = 0;
    for(unsigned int i = row - left;i <= row + down;i++) {
        for(unsigned int j = col - up;j <= col + right;j++) {
            if (i == row && j == col) continue;
            if((*curr)[i][j]) {

                v.push_back({i,j}); // CHECK IF COPY_CONSTRUCTOR
            }
        }
    }

    return v;
}

void Game::update1(unsigned int row,unsigned int col) {
    vector<vector<unsigned int>> v = livingNeighbors(row,col);
    unsigned int currVal = (*curr)[row][col];

    (*next)[row][col] = currVal; // Default
    unsigned int numLiving = v.size();

    if(currVal) { // Alive
        if(numLiving < 2 || numLiving > 3)
            (*next)[row][col] = DEAD;


        return;
    }
    //Dead
    if(numLiving != 3) return;
    unsigned int spec[SPECIES];
    for(int i = 0; i < numLiving;i++)
        spec[(*curr)[v[i][0]][v[i][1]] - 1] += (*curr)[v[i][0]][v[i][1]];
    unsigned int dominantIndex,dominantVal = 0 ;

    for(int i = 0; i < SPECIES;i++)
        if(spec[i] > dominantVal) {
            dominantIndex = i;
            dominantVal = spec[i];
        }

    (*next)[row][col] = dominantIndex + 1;
}

void Game::update2(unsigned int row,unsigned int col) {


    unsigned int spec = (*curr)[row][col];
    if(spec == DEAD) {
        (*next)[row][col] = DEAD;
        return;
    }
    vector<vector<unsigned int>> v = livingNeighbors(row,col);
    unsigned int  numLiving = v.size();
    for(unsigned int i = 0; i < numLiving;i++)
        spec += (*curr)[v[i][0]][v[i][1]];

    (*next)[row][col] = round((double)spec/(1+numLiving));

}

void Game::phase1(unsigned int startLine,unsigned  int endLine){
     cout << "Phase1 : " << endl;

       for (unsigned int i = 0; i < matrixSize[0]; i++)
       {
           for (unsigned int j = 0; j < matrixSize[1]; j++)
           {
               cout << (*curr)[i][j] << " ";
           }

           // Newline for new row
           cout << endl;
       }

       cout<< endl<< endl;
/*
       for (unsigned int i = 0; i < matrixSize[0]; i++)
       {
           for (unsigned int j = 0; j < matrixSize[1]; j++)
           {
               cout << livingNeighbors(i,j).size() << " ";
           }

           // Newline for new row
           cout << endl;
       }

       cout << endl;
   */



    for(unsigned int i = startLine; i <= endLine; i++) {
        for(unsigned int j = 0;j < matrixSize[1]; j++)
            update1(i,j);
    }
}

void Game::phase2(unsigned int startLine, unsigned int endLine){


cout << "Phase2 : " << endl;
    for (unsigned int i = 0; i < matrixSize[0]; i++)
    {
        for (unsigned int j = 0; j < matrixSize[1]; j++)
        {
            cout << (*curr)[i][j] << " ";
        }

        // Newline for new row
        cout << endl;
    }

    cout<< endl<< endl;




    for(unsigned int i = startLine; i <= endLine; i++) {
        for(unsigned int j = 0;j < matrixSize[1]; j++)
            update2(i,j);
    }



    cout << "Phase3 : " << endl;

    for (unsigned int i = 0; i < matrixSize[0]; i++)
    {
        for (unsigned int j = 0; j < matrixSize[1]; j++)
        {
            cout << (*next)[i][j] << " ";
        }

        // Newline for new row
        cout << endl;
    }

    cout<< endl<< endl;








}
void Game::_init_game() {

    vector<string> stringLines = utils::read_lines(g.filename);

    totPops = 0 ;

    vector<string> stringCol;
    vector<unsigned int> temp;
    curr = new vector<vector<unsigned int>>;
    next = new vector<vector<unsigned int>>;
    matrixSize[0] =  stringLines.size();
    m_gen_num = g.n_gen;
	interactive_on = g.interactive_on; // Controls interactive mode - that means, prints the board as an animation instead of a simple dump to STDOUT 
	print_on = g.print_on;
    m_thread_num = min(matrixSize[0],g.n_thread);

    for(unsigned int line = 0; line < matrixSize[0]; line++) {
        stringCol = utils::split(stringLines[line], ' ');
        matrixSize[1] = stringCol.size();

        for (unsigned int col = 0; col < matrixSize[1]; col++){

            temp.push_back(std::stoi(stringCol[col])) ;

        }
        curr->push_back(temp);
        next->push_back(temp);
        temp.clear();
    }


    // Create & Start threads
    pthread_mutex_init(&lock,NULL);
    pthread_cond_init(&fin,NULL);
    pthread_mutex_init(&popLock,NULL);

    for (unsigned int i = 0 ; i < m_thread_num; i++) {
        Tile *t = new Tile(i, this);
        t->start();

        m_threadpool.push_back(t);
    }

    // Testing of your implementation will presume all threads are started here

}

void Game::swapMatrices() {
    vector<vector<unsigned int>>* temp = curr;
    curr = next;
    next = temp;
}
void Game::_step(uint curr_gen) {

    finished = 0;
    Task *t;
    unsigned int i,rowPerTile = matrixSize[0]/m_thread_num;
    // Start Timer
    cout << "IM HERE : phase 1" << endl;

    // Phase 1
    for (i = 0 ; i < m_thread_num-1; i++) {


        t = new Task(this,i*rowPerTile,(i+1)*rowPerTile - 1,1);
        tasksToThreads.push(t);
    }








    t = new Task(this,i*rowPerTile,matrixSize[0] - 1,1);
    tasksToThreads.push(t);


    // Wait for the workers to finish calculating
    pthread_mutex_lock(&lock);
    while(!phaseFinish())
        pthread_cond_wait(&fin,&lock);
    finished = 0;
    pthread_mutex_unlock(&lock);

    // Swap pointers between current and next field
    swapMatrices();



    cout << "IM HERE : phase 2" << endl;

    // Phase 2
    for (i = 0 ; i < m_thread_num - 1; i++) {
        t = new Task(this,i*rowPerTile,(i+1)*rowPerTile - 1,2);
        tasksToThreads.push(t);
    }
    t = new Task(this,i*rowPerTile,matrixSize[0] - 1,2);
    tasksToThreads.push(t);



    // Wait for the workers to finish calculating
    pthread_mutex_lock(&lock);
    while(!phaseFinish())
        pthread_cond_wait(&fin,&lock);
    finished = 0;
    pthread_mutex_unlock(&lock);

	// Swap pointers between current and next field
    swapMatrices();



    // NOTE: Threads must not be started here - doing so will lead to a heavy penalty in your grade
}

void Game::_destroy_game(){
	// Destroys board and frees all threads and resources 
	// Not implemented in the Game's destructor for testing purposes. 
	// All threads must be joined here
    delete curr;
    delete next;
	for (uint i = 0; i < m_thread_num; ++i) {
        m_threadpool[i]->join();
        delete m_threadpool[i];
    }
    pthread_mutex_destroy(&lock);

}

/*--------------------------------------------------------------------------------
								
--------------------------------------------------------------------------------*/
inline void Game::print_board(const char* header) {

	if(print_on){
    unsigned int field_height = matrixSize[0];
    unsigned int field_width = matrixSize[1];
		// Clear the screen, to create a running animation 
		if(interactive_on)
			system("clear");

		// Print small header if needed
		if (header != nullptr)
			cout << "<------------" << header << "------------>" << endl;

        cout << u8"╔" << string(u8"═") * field_width << u8"╗" << endl ;
        for (uint i = 0; i < field_height; ++i) {
            cout << u8"║";
            for (uint j = 0; j < field_width; ++j) {
                if ((*curr)[i][j] > 0)
                    cout << colors[(*curr)[i][j] % 7] << u8"█" << RESET;
                else
                    cout << u8"░";
            }
            cout << u8"║" << endl;
        }
        cout << u8"╚" << string(u8"═") * field_width << u8"╝" << endl;

		// Display for GEN_SLEEP_USEC micro-seconds on screen 
		if(interactive_on)
			usleep(GEN_SLEEP_USEC);
	}

}


/* Function sketch to use for printing the board. You will need to decide its placement and how exactly 
	to bring in the field's parameters. 

		cout << u8"╔" << string(u8"═") * field_width << u8"╗" << endl;
		for (uint i = 0; i < field_height ++i) {
			cout << u8"║";
			for (uint j = 0; j < field_width; ++j) {
                if (field[i][j] > 0)
                    cout << colors[field[i][j] % 7] << u8"█" << RESET;
                else
                    cout << u8"░";
			}
			cout << u8"║" << endl;
		}
		cout << u8"╚" << string(u8"═") * field_width << u8"╝" << endl;
*/
