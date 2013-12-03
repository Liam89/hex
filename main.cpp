// Terminal Window based hex game developed as part of Coursera's C++ for C programmers

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <deque>
#include <cmath>
#include <limits>
#include <algorithm>
#include <set>
#include <regex>
#include <cassert>

using namespace std;


//which set a Node is in
enum set_t{NONE,OPEN,CLOSED};
//the color of a Node, i.e. whether it is taken by RED/BLUE
enum cell{EMPTY, RED, BLUE};
//representation of cell type when printed to terminal window.
const char cell_repr[3] = {'.', 'R', 'B'};


struct Player {
    Player(string name, cell color) : name(name), color(color), AI(false) {}
    Player(){}
    string name;
    cell color;
    bool AI;
};

//Node in the Graph
class Node {
    public:
        Node(int node_index) : set_type(NONE), node_index(node_index), distance(0) {}
        Node(int node_index, int distance):set_type(NONE),node_index(node_index),distance(distance){}

        int get_index() { return node_index; }
        double get_distance() { return distance; }
        int get_prev() { return previous_node; }
        void set_distance(double dist, int prev_node = -1) { //-1 indicates no previous node
            distance = dist;
            previous_node = prev_node;
        }
        //reinitialise to default values
        void reset() {
            set_type = NONE;
            distance = 0;
        }
        bool operator==(Node& rhs) const{
            return node_index == rhs.get_index();
        }
        bool operator<(Node& rhs) const{
            return (distance == rhs.get_distance()) ? node_index < rhs.get_index() :
                                                        distance < rhs.get_distance();
        }
        friend class NodeList; //allows NodeList to have access to private members in Node

    protected:
        set_t set_type;
        int node_index;     //nodes in graph are named via index from 0 -> n-1
        int previous_node;  //previous_node is neighbour node traversed from when adding Node to closed set
        double distance;
};

//comparison operator used to sort by Node rather than pointer
struct compare_ptr{
    bool operator() (Node* node_ptr1, Node* node_ptr2) {
            return *node_ptr1 < *node_ptr2;
    }
};

//maintains the list of nodes and the open set
class NodeList {
    public:
        NodeList() {}
        NodeList(int num_nodes) : num_nodes(num_nodes) {
            for(int i = 0; i < num_nodes; ++i) node_list.push_back( Node(i) );
        }
        //functions to get the distance and set of node i
        set_t set_type(int i) { return node_list[i].set_type; }
        double get_distance(int i) { return node_list[i].distance; }
        // reset all nodes to default values
        void reset_node_list() {
            for(int j = 0; j < num_nodes; ++j) {
                node_list[j].reset();
            }
        }
        //clear open set and init open set with node i
        void init_open_set(int i){
            open_set.clear();
            open_set.insert(&node_list[i]);
            node_list[i].set_type=OPEN;
        }
        //move node with min distance from open set to closed set and return pointer to node
        Node* get_min_from_open_set();
        //add node j to open set, i is the node that led to j. new_dist currently has no use
        void add_node_to_open_set(int j, int i, double new_dist);
        size_t open_set_size(){return open_set.size();}

    private:
        int num_nodes;
        //list of all Nodes in the Graph
        vector<Node> node_list;
        set<Node*,compare_ptr> open_set;
        //remove j from open set
        void open_set_delete(int j){
            auto it = open_set.find(&node_list[j]);
            open_set.erase( it );
        }
        //insert/update j in open set
        void open_set_insert(int j, double new_dist){
            assert(node_list[j].set_type == OPEN && "set not open");
            auto ret = open_set.insert(&node_list[j]);
            assert(ret.second == true);
        }
};

//moves node with min distance from open set to closed set, and returns pointer to node
Node* NodeList::get_min_from_open_set() {
    //get min Node in open set
    Node tmp_min(0,0);
    set<Node*>::iterator min_node_it = open_set.lower_bound(&tmp_min); //no negative weighted edges so returns iterator to min element
    Node* min_node = *min_node_it;
    //move from open to closed set
    min_node->set_type = CLOSED;
    open_set.erase(min_node_it);
    return min_node;
}

//j is node to add, i is node leading to j, new_dist is distance to node j from starting node
void NodeList::add_node_to_open_set(int j, int i, double new_dist) {
    //if node j is not in the open set add it
    if (node_list[j].set_type != OPEN) {
        node_list[j].set_type = OPEN;
        node_list[j].set_distance(new_dist, i);
        open_set_insert(j,new_dist);
    //if node j is in the open set update the distance if it is shorter
    } else if (new_dist < node_list[j].get_distance()){
        open_set_delete(j);
        node_list[j].set_distance(new_dist, i);
        open_set_insert(j, new_dist);
    }
}



//Undirected Graph - by default unweighted
//Contains both matrix representation of connection between graph nodes for fast access to edges and adjacency list for fast access to neighbours.
template<class T>
class Graph {
    public:
        Graph(int num_nodes, T val_for_no_edge = false) : num_nodes(num_nodes), val_for_no_edge(val_for_no_edge),
            graph(num_nodes, vector<T>(num_nodes, val_for_no_edge)), neighbours(num_nodes, vector<int>(0)) {} //graph is in row major
        //checks whether there's an edge between cell(row1,col1) and cell(row2, col2). row and col are zero based
        T edge_between_nodes(int node1, int node2) {
            return graph[node1][node2];
        }
    protected:
        int num_nodes;
        T val_for_no_edge;
        vector< vector<bool> > graph;
        vector< vector<int> > neighbours;   //vector with outer index representing node number and inner vector of neighbours to the node
        void set_edge(int node1, int node2, T weight = true){
            graph[node1][node2] = graph[node2][node1] = weight;
        }
        void collapse_matrix();
};

template<class T>
void Graph<T>::collapse_matrix() {
    //collapses each row of matrix into vector of neighbours within neighbours[node]
    //mcol/mrow refers to matrix not board
    for(int mrow = 0; mrow < num_nodes; ++mrow) {
        for(int mcol = 0; mcol < num_nodes; ++mcol) {
            if (graph[mrow][mcol]) neighbours[mrow].push_back(mcol);
        }
    }
}

class HexGraph: private Graph<bool> {
    public:
        HexGraph(int num_cols, int num_rows) : Graph(num_cols*num_rows), num_cols(num_cols), num_rows(num_rows) {} //graph is in row major
        //checks whether there's an edge between cell(row1,col1) and cell(row2, col2). row and col are zero based
        bool edge_between_cells(int row1, int col1, int row2, int col2) {
            return graph[row1*num_cols + col1][row2*num_cols + col2];
        }
        void gen();
    private:
        int num_cols, num_rows;
        void set_edge_hex(int row1, int col1, int row2, int col2){
            set_edge(row1*num_cols + col1, row2*num_cols + col2);
        }
};

/*generate empty board that looks as follows. A 'column' proceeds to node below to the right.
 0   1   2   3
0 . - . - . - .
   \ / \ / \ / \
1   . - . - . - .
     \ / \ / \ / \
2     . - . - . - .
       \ / \ / \ / \
3       . - . - . - .
*/
void HexGraph::gen() {
    for(int row = 0; row < num_rows; ++row) {
        for(int col = 0; col < num_cols; ++col) {
            //create edge to node on right (and by extension to node on left)
            if (col != (num_cols-1)) set_edge_hex(row, col, row, col+1);
            //create edge to node below (and by extension to node above)
            if (row != (num_rows-1)) set_edge_hex(row, col, row+1, col);
            //create edge to node below and to left (and by extension above and to right)
            if (col != 0 && row != (num_rows-1)) set_edge_hex(row, col, row+1, col-1);
        }
    }
    //generate adjacency list of neighbours
    collapse_matrix();
}


class Board {
public:
    Board(int s) : board_size(s), board(s, vector<cell>(s, EMPTY)), graph(s,s), node_list(s*s) {
        graph.gen();
    }
    void play();
    void render();

private:
    int board_size; //length of row/column
    vector< vector<cell> > board;   //cell is '.','R', or 'B'
    HexGraph graph;
    vector<Player> players;
    NodeList node_list;
    Player current_player;

    bool update_board(int row, int col);
    //execute players turn
    void turn(int player_num);
    void add_neighbours_to_open_set(int i);
    void find_path_to(int final_node);
    void find_all_paths_from(int initial);
    bool winning_move(int row, int col, int player_num);
    bool horizontal_path();
    bool vertical_path();
    int ai_move(int player_num);
};

//Tries to take cell(row, col) and returns whether successful
bool Board::update_board(int row, int col) {
    if (board[row][col] != EMPTY) return false;
    board[row][col] = current_player.color;
    return true;
}

//Print playing board to console
void Board::render() {
/*generate empty board that looks as follows. A 'column' proceeds to node below and to the right.
 0   1   2   3
0 . - . - . - .
   \ / \ / \ / \
1   . - . - . - .
     \ / \ / \ / \
2     . - . - . - .
       \ / \ / \ / \
3       . - . - . - .
*/
    int rows = board_size, cols = board_size;
    string spaces;
    //print numbers at top
    cout << " ";
    for(int c = 0; c < cols; ++c) {
        cout << c << "   ";
    }
    cout << "\n";

    for(int r = 0; r < rows-1; ++r) {
        //print row
        cout << spaces << r << " ";
        for(int c = 0; c < cols-1; ++c) {
            cout << cell_repr[board[r][c]] << " - ";
        }
        cout << cell_repr[board[r][cols-1]] << "\n";
        //print connectors to row below
        cout << "   " << spaces;
        for(int c = 0; c < cols-1; ++c) {
            cout << "\\ / ";
        }
        cout << "\\\n";
        spaces += "  ";
    }
    //print final row
    cout << spaces << rows-1 << " ";
    for(int c = 0; c < cols-1; ++c) {
        cout << cell_repr[board[rows-1][c]] << " - ";
    }
    cout << cell_repr[board[rows-1][cols-1]] << "\n";
}

//Set up players in game
void Board::play() {
    string name;
    cout << "Enter name of Player 1 (B), type \"AI\" for AI" << endl;
    cin >> name;
    players.push_back(Player(name, BLUE));
    if (players[0].name == "AI") players[0].AI = true;
    cout << "Enter name of Player 2 (R), type \"AI\" for AI" << endl;
    cin >> name;
    cout << endl;
    players.push_back(Player(name, RED));
    if (players[1].name == "AI") players[1].AI = true;
    //start game with blue's turn
    turn(0);
}

void Board::turn(int player_num) {
    string input;
    bool success;
    int row,col;
    current_player = players[player_num];
    if (current_player.AI) {
        cout << "\n" << current_player.name << "'s turn." << endl;
        render();
        cout << "\n" << current_player.name << "'s thinking." << endl;
        int node = ai_move(player_num);
        row = node/board_size;
        col = node % board_size;
        //checks whether move's valid and continues play
        success = update_board(row,col);
    } else {
        cout << "\n" << current_player.name << "'s turn. Input using: row,col  e.g. \'0,1\'    type \'exit\' to exit\n" << endl;
        render();
        //get player's move
        cin >> input;
        if (input == "exit") return;
        //converts first number in string (input) to int (row).
        row = atoi(input.c_str());
        //removes first number and ,
        input = input.substr(input.find(',') + 1);
        col = atoi(input.c_str());
        //checks whether move's valid and continues play
        success = update_board(row,col);
        while (!success) {
            cout << "\n!!!Invalid move!!!\n" << endl;
            cout << "\n" << current_player.name << "'s turn. Input using: row,col  e.g. \'0,1\'    type \'exit\' to exit\n" << endl;
            render();
            cin >> input;
            if (input == "exit") return;
            row = atoi(input.c_str());
            input = input.substr(input.find(',') + 1);
            col = atoi(input.c_str());
            success = update_board(row,col);
        }
    }
    if (winning_move(row, col, player_num)) {
        cout << endl;
        render();
        cout << "!!!" << current_player.name << " wins!!!" << endl;
        return;
    }
    turn((player_num + 1) % 2); //cycle to other player
}

//checks whether move to node (row,col) by player_num wins game
bool Board::winning_move(int row, int col, int player_num) {
    int node = row*board_size + col;
    //perform recursive search from node to all possible nodes. If there's a path to a node it's set_type will be changed to CLOSED
    find_all_paths_from(node);
    //check whether path reaches nodes at opposite edges of board. i.e. whether the last move created a winning path
    if (player_num == 0)
        return horizontal_path();
    return vertical_path();
}

//Check whether path reaches nodes at opposite edges of board. i.e. whether there's a horizontal path
bool Board::horizontal_path() {
    int col = 0, node;
    bool found_left = false;
    //Check whether path was found to left-most col of board
    for(int row = 0; row < board_size; ++row) {
        node = row*board_size + col;
        if (node_list.set_type(node) == CLOSED) {
            found_left = true;
            break;
        }
    }
    if (!found_left) return false;
    //If path to left node found, check for path to right-most col
    col = board_size - 1;
    for(int row = 0; row < board_size; ++row) {
        node = row*board_size + col;
        if (node_list.set_type(node) == CLOSED) return true;
    }
    return false;

}
//check whether path reaches nodes at opposite edges of board. i.e. whether there's a vertical path
bool Board::vertical_path() {
    int row = 0,node;
    bool found_top = false;
    //check whether path was found to left-most col of board
    for(int col = 0; col < board_size; ++col) {
        node = row*board_size + col;
        if (node_list.set_type(node) == CLOSED) {
            found_top = true;
            break;
        }
    }
    if(!found_top) return false;
    //if path to left node found, check for path to right-most col
    row = board_size - 1;
    for(int col = 0; col < board_size; ++col){
        node = row*board_size + col;
        if(node_list.set_type(node) == CLOSED) return true;
    }
    return false;

}


//function to add neighbours of node i to the open set, helper function for find_path_to()
void Board::add_neighbours_to_open_set(int i){
    int col1 = i % board_size, row1 = i / board_size;
    for(int j = 0; j < board_size * board_size; ++j){ //loop over nodes in graph to find edges //TODO: change to +-1
        int col2 = j % board_size, row2 = j / board_size;
        if( graph.edge_between_cells(row1, col1, row2, col2) && (node_list.set_type(j) != CLOSED) && (board[row2][col2] == current_player.color) ){
            double dist = node_list.get_distance(i) + 1; //not currently needed but kept incase needed for next homework
            //update/add node (row,col) to open set
            node_list.add_node_to_open_set(j, i, dist);
        }
    }
}

//breadth first search to final_node (modified from Dijkstra's, find_all_paths_from and find_path_to kept separate for posterity)
//finds path to final_node
void Board::find_path_to(int final_node) {
    if(node_list.open_set_size() == 0)
        return; //return if no way from start to end node
        //move top node from open set to closed set
    Node* next_node = node_list.get_min_from_open_set();
    int index = (*next_node).get_index();
        //add neighbours to index to open set
    add_neighbours_to_open_set(index);
        //return if reached end node
    if (index == final_node)
        return;
        //recurse until reach end node or can't reach any more nodes
    return find_path_to(final_node);

}

//breadth first search to from initial to all possible nodes
//Tries to find paths from initial node to all other nodes. If path found node's set_type will be CLOSED
void Board::find_all_paths_from(int initial) {
    //reset nodes in node list to initial values
    node_list.reset_node_list();
    //add initial node to open set
    node_list.init_open_set(initial);
    for(int i = 0; i < board_size*board_size; ++i){
        //if in closed set, path to node i has already been found
        if(node_list.set_type(i) != CLOSED)
                find_path_to(i);
    }
}

//monte carlo ai
int Board::ai_move(int player_num) {
    vector<int> possible_moves_orig; //possible moves identified by Node index
    vector<int> num_wins(board_size*board_size,0);
    for(int row=0; row < board_size; ++row){
        for(int col = 0; col < board_size; ++col){
            if(board[row][col] == EMPTY) possible_moves_orig.push_back(row*board_size + col);
        }
    }
    //perform monte carlo ~1000 times per possible move
    for(int i = 0; i < 1000*possible_moves_orig.size(); ++i) {
        vector<int> possible_moves_copy = possible_moves_orig;
        bool win=false;
        random_shuffle ( possible_moves_copy.begin(), possible_moves_copy.end() );
        int first_move = possible_moves_copy[0];
        //perform the current ai's moves randomly, don't need to actually make other players moves
        for(int k = 0; k < (possible_moves_copy.size()+1)/2; ++k){
            int row = possible_moves_copy[k]/board_size, col = possible_moves_copy[k] % board_size;
            board[row][col]=current_player.color;
            if( (win = winning_move(row,col,player_num)) ){
                num_wins[first_move]++;
                //undo moves
                for(int j = 0; j<k+1; ++j){
                    row = possible_moves_copy[j]/board_size;
                    col = possible_moves_copy[j] % board_size;
                    board[row][col]=EMPTY;
                }
                break;
            }
        }
        if(!win){
            for(int j = 0; j<(possible_moves_copy.size()+1)/2; ++j){
                int row = possible_moves_copy[j]/board_size;
                int col = possible_moves_copy[j] % board_size;
                board[row][col]=EMPTY;
            }
        }
    }
    int max_el = static_cast<int>(max_element(num_wins.begin(),num_wins.end()) - num_wins.begin());
    return max_el;
}

/*  //not compatible with compiler TODO: without regex
bool input_convert(string input, int& row, int& col){
    regex e ("^([A-Za-z]{1})([0-9]+)$");  //match 1 letter followed by 1 or more digits
    smatch sm;  //holds results of match
    if (!regex_match(input, sm, e)) return false;
    string srow = sm.str(1);
    row = static_cast<int>(srow[0] - 'A');
    col = atoi(sm.str(2).c_str());
    return true;

}*/


int main()
{
    srand (unsigned (time(0)));
    cout << "Please enter board size, e.g. 11 for 11x11:" << endl;
    int board_size;
    cin >> board_size;
    Board board(board_size);
    board.play();

    return 0;
}
