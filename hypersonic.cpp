#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

struct Position
{
    Position(int r, int c): row(r), col(c)
    {}

    bool operator==(Position const& other) const
    {
        return other.row == row && other.col == col;
    }

    bool operator!=(Position const& other) const
    {
        return !(*this == other);
    }

    int row;
    int col;
};

struct Field
{
    static Field& get()
    {
        static Field f;
        return f;
    }

    void clear()
    {
        previous_field_ = field_;
        field_.clear();
    }

    void update_rows(string row)
    {
        field_.push_back( vector<char>(row.begin(), row.end()) );
    }

    void set_character_pos(const Position& p)
    {
        char_pos = p;
        field_[p.row][p.col] = Symbol::character;
    }

    void set_range_upgrade(const Position& p)
    {
        field_[p.row][p.col] = Symbol::range_upgrade;
    }
    
    bool picked_up_range_upgrade(const Position& p)
    {
        if( !previous_field_.empty() ) // game has started
        {
            return previous_field_[p.row][p.col] == Symbol::range_upgrade;
        }
    }

    void set_count_upgrade(const Position& p)
    {
        field_[p.row][p.col] = Symbol::count_upgrade;
    }
    
    bool picked_up_count_upgrade(const Position& p)
    {
        if( !previous_field_.empty() ) // game has started
        {
            return previous_field_[p.row][p.col] == Symbol::count_upgrade;
        }
    }

    void set_bomb(const Position& p, int range)
    {
        if( field_[p.row][p.col] != Symbol::bomb ) // register only new
        {
            field_[p.row][p.col] = Symbol::bomb;
            update_bomb_affected_boxes( p, range );
        }
    }
    
    bool is_in_blast_range(const Position& p)
    {
        return field_[p.row][p.col] == Symbol::blast;
    }

    void update_bomb_affected_boxes(const Position& p, int range )
    {
        auto found_in_range = [&]( const Position& p ) -> bool
        {
            if( !is_in_field( p ) ) return true;
            if( field_[p.row][p.col] == Symbol::wall || field_[p.row][p.col] == Symbol::range_upgrade || field_[p.row][p.col] == Symbol::count_upgrade ) return true; // no blast behind a wall or items
            if( is_field_box( field_[p.row][p.col] ) ) { field_[p.row][p.col] = Symbol::box_blasted; return true; } // count boxes, but no blast behind it
            
            field_[p.row][p.col] = Symbol::blast;
            return false; // continue search
        };
        
        // rows
        for( int r = p.row + 1; r <= p.row + range; r++ )
        {
            if( found_in_range( {r, p.col} ) ) break;
        }
        for( int r = p.row - 1; r >= p.row - range; r-- )
        {
            if( found_in_range( {r, p.col} ) ) break;
        }
        
        // cols
        for( int c = p.col + 1; c <= p.col + range; c++ )
        {
            if( found_in_range( {p.row, c} ) ) break;
        }
        for( int c = p.col - 1; c >= p.col - range; c-- )
        {
            if( found_in_range( {p.row, c} ) ) break;
        }
    }

    vector<Position> get_closest_box_from(Position p, int range = 0)
    {
        // Breadth First Search
        vector<Position> ret;
        const int limit = 5;
        queue<Position> q;
        q.push(p);
        vector<vector<char>> field_copy_ = field_;

        while( !q.empty() )
        {
            Position next = q.front();
            q.pop();

            cerr << "Pos " << next.col << ":" << next.row << endl;
            if( !is_in_field( next ) )
            {
                //cerr << "Erroneous pos" << endl;
                continue;
            }
            
            if( !has_path( p, next ) )
            {
                //cerr << "No path" << endl;
                continue;
            }

            if( is_field_box( field_copy_[next.row][next.col] ) )
            {
                cerr << "Pos found" << endl;
                ret.push_back( next );
                
                if( ret.size() == limit )
                {
                    break;
                }
                
                continue;
            }

            if( field_copy_[next.row][next.col] == Symbol::wall )
            {
                //cerr << "Pos has a wall" << endl;
                continue;
            }
            
            if( field_copy_[next.row][next.col] == Symbol::box_blasted )
            {
                //cerr << "Pos has a box_blasted" << endl;
                continue;
            }
            
            if( range && ( abs( next.row - p.row ) <= range || abs( next.col - p.col) <= range ) )
            {
                cerr << "Pos will be affected by future bomb" << endl;
                field_copy_[next.row][next.col] = Symbol::box_blasted;
                //continue;
            }
            
            if( field_copy_[next.row][next.col] == Symbol::processed )
            {
                //cerr << "Pos is processed" << endl;
                continue;
            }
            field_copy_[next.row][next.col] = Symbol::processed; //processed
            //cerr << print() << endl;

            q.push({next.row-1, next.col});
            q.push({next.row, next.col-1});
            q.push({next.row+1, next.col});
            q.push({next.row, next.col+1});
        }

        return ret;
    }

    Position best_place_to_bomb_around( const Position& p, const int range )
    {
        auto count_boxes_in_blast = [&]( const Position& p ) -> int
        {
            int boxes = 0;
            
            if( is_field_box( field_[p.row][p.col] ) || field_[p.row][p.col] == Symbol::wall )
            {
                // can't place on box
                return 0;
            }
            
            auto found_in_range = [&]( const Position& p ) -> bool
            {
                if( !is_in_field( p ) ) return true;
                if( field_[p.row][p.col] == Symbol::wall || field_[p.row][p.col] == Symbol::range_upgrade || field_[p.row][p.col] == Symbol::count_upgrade ) return true; // no blast behind a wall or items
                if( is_field_box( field_[p.row][p.col] ) ) { boxes ++; return true; } // count boxes, but no blast behind it
                return false; // continue search
            };
            
            // rows
            for( int r = p.row + 1; r <= p.row + range; r++ )
            {
                if( found_in_range( {r, p.col} ) ) break;
            }
            for( int r = p.row - 1; r >= p.row - range; r-- )
            {
                if( found_in_range( {r, p.col} ) ) break;
            }
            
            // cols
            for( int c = p.col + 1; c <= p.col + range; c++ )
            {
                if( found_in_range( {p.row, c} ) ) break;
            }
            for( int c = p.col - 1; c >= p.col - range; c-- )
            {
                if( found_in_range( {p.row, c} ) ) break;
            }

            return boxes;
        };
        
        auto applicable = [&]( const Position& p ) -> bool
        {
            //  and not crate/wall and has path to my char
            return field_[p.row][p.col] != Symbol::character && // not Cracracter's current pos, as currently bomb is being placed here
                   !is_obstacle( field_[p.row][p.col] ) && // not a box/wall so that bomb can be places
                   field_[p.row][p.col] != Symbol::blast && // do not stand on other bomb's blast range
                   has_path( char_pos, p ); // pathi to this position is clear
        };

        int best_count = 0;
        Position best_pos = Position(-1, -1);
        for( int r = p.row - range; r <= p.row + range; r++ )
        {
            if(r < 0 || r >= field_.size() || r == p.row ) continue;
            int boxes = count_boxes_in_blast({r, p.col});

            if( boxes > best_count && applicable( {r, p.col} ) )
            {
                cerr << "Position " << p.col << " " << r << " has boxes: " << boxes << endl;
                best_count = boxes;
                best_pos = Position(r, p.col);
            }
        }

        for( int c = p.col - range; c <= p.col + range; c++ )
        {
            if(c < 0 || c >= field_[0].size() || c == p.col ) continue;
            int boxes = count_boxes_in_blast({p.row, c});

            if( boxes > best_count && applicable( {p.row, c} ) )
            {
                cerr << "Position " << c << " " << p.row << " has boxes: " << boxes << endl;
                best_count = boxes;
                best_pos = Position(p.row, c);
            }
        }

        return best_pos;
    }

    string print()
    {
        string res;
        for(const auto& row: field_)
        {
            res += string(row.begin(), row.end()) + "\n";
        }
        return res;
    }

    vector<vector<char>> field_;
    vector<vector<char>> previous_field_;

    enum Symbol
    {
        empty = '.',
        processed = ',',
        character = '!',
        box = '0',
        box_witn_range = '1',
        box_witn_bomb = '2',
        box_blasted = 'a',
        bomb = '@',
        blast = '#',
        range_upgrade = 'R',
        count_upgrade = 'C',
        wall = 'X'
    };

private:
    Field() {}
    Position char_pos = {-1, -1};
    
    bool is_in_field( const Position& p ) const
    {
        return p.row >= 0 && p.row < field_.size() && p.col >= 0 && p.col < field_[0].size();
    }

    const vector<char> field_boxes = {box, box_witn_range, box_witn_bomb}; // excluding blasted boxes
    bool is_field_box( char c ) const
    {
        return find( field_boxes.begin(), field_boxes.end(), c ) != field_boxes.end();
    }
    
    bool is_obstacle( char c ) const
    {
        return is_field_box( c ) || c == Symbol::wall || c == Symbol::box_blasted || c == Symbol::bomb;
    }
    
    bool has_path( const Position& from, const Position& to ) const
    {
        if( !is_in_field( from ) || !is_in_field( to ) )
        {
            return false;
        }
        
        queue<Position> q;
        q.push( from );
        vector<vector<char>> field_copy_ = field_;

        while( !q.empty() )
        {
            Position next = q.front();
            q.pop();
            
            if( next == to )
            {
                return true;
            }

            //cerr << "Pos " << next.row << ":" << next.col << endl;
            if( !is_in_field( next ) )
            {
                //cerr << "Erroneous pos" << endl;
                continue;
            }

            if( is_obstacle( field_copy_[next.row][next.col] ) )
            {
                //cerr << "no path" << endl;
                continue;
            }

            if( field_copy_[next.row][next.col] == Symbol::processed )
            {
                //cerr << "Pos was processed before" << endl;
                continue;
            }
            field_copy_[next.row][next.col] = Symbol::processed; //processed

            q.push({next.row-1, next.col});
            q.push({next.row, next.col-1});
            q.push({next.row+1, next.col});
            q.push({next.row, next.col+1});
        }
        
        return false;
    }
};

struct Character
{
    static Character& get()
    {
        static Character c;
        return c;
    }

    Position my_pos = nowhere;

    void set_next_pos( bool will_be_bombed = false )
    {
        vector<Position> closest_boxes = Field::get().get_closest_box_from( my_pos, ( will_be_bombed ? bomb_range : 0 ) );
        
        for( const auto& closest_box: closest_boxes )
        {
            cerr << "Closest "<< closest_box.col << " " << closest_box.row << endl;
            Position best = Field::get().best_place_to_bomb_around( closest_box, bomb_range );
            cerr << "Best "<< best.col << " " << best.row << endl;
            if( best == nowhere )
            {
                cerr << "Not optimal" << endl;
            }
            else
            {
                next_pos = best;
            }
        }
        cerr << "Going nowhere" << endl;
    }
    

    void bomb_and_move()
    {
        update_bomb_timers();
        
        if( next_pos == nowhere )
        {
            // Init move
            set_next_pos();
        }
        
        if( Field::get().is_in_blast_range(next_pos) )
        {
            // update with newly placed bombs
            set_next_pos();
        }
        
        cerr << "My pos "<< my_pos.col << " " << my_pos.row << endl;
        cerr << "Next pos "<< next_pos.col << " " << next_pos.row << endl;
        cerr << "Bombs left "<< bombs << endl;
        if( my_pos == next_pos && bombs )
        {
            set_next_pos( true );
            bombs--;
            bomb_timers.push_back(8);
            cerr << "New Next pos "<< next_pos.col << " " << next_pos.row << endl;
            cout << "Bomb "<< next_pos.col << " " << next_pos.row << endl;
        }
        else
        {
            cerr << "Next pos "<< next_pos.col << " " << next_pos.row << endl;
            cout << "Move "<< next_pos.col << " " << next_pos.row << endl;
        }
    }

    void range_upgrade()
    {
        bomb_range++;
        cerr << "range_upgrade: " << bomb_range << endl;
    }

    void count_upgrade()
    {
        bomb_count++;
        bombs++;
        cerr << "count_upgrade: " << bomb_count << endl;
    }
    
    void check_pickups()
    {
        if( Field::get().picked_up_range_upgrade( my_pos ) )
        {
            range_upgrade();
        }
        
        if( Field::get().picked_up_count_upgrade( my_pos ) )
        {
            count_upgrade();
        }
    }

private:
    Character() {}
    const Position nowhere = {-1, -1};
    Position next_pos = nowhere;

    int bomb_range = 2;
    int bomb_count = 1;
    int bombs = 1;
    deque<int> bomb_timers;
    
    void update_bomb_timers()
    {
        for_each( bomb_timers.begin(), bomb_timers.end(), [&]( int& timer ) { 
            timer--; 
            cerr << "timer: " << timer << endl;
        });
        while( !bomb_timers.empty() && bomb_timers.front() < 0 ) // 8 rounds inclusive, thus 0 means bomb still on field
        {
            bomb_timers.pop_front();
            bombs++;
        }
    }
};

enum Entities
{
    character = 0,
    bomb = 1,
    item = 2
};

int main()
{
    int width;
    int height;
    int myId;
    cin >> width >> height >> myId; cin.ignore();
    //cerr << "myId: " << myId << endl;

    // game loop
    while (1) {
        Field::get().clear();
        for (int i = 0; i < height; i++) {
            string row;
            cin >> row; cin.ignore();
            //cerr << "Row: " << row << endl;
            Field::get().update_rows(row);
        }

        int entities;
        cin >> entities; cin.ignore();
        for (int i = 0; i < entities; i++) {
            int entityType;
            int owner;
            int x;
            int y;
            int param1;
            int param2;
            cin >> entityType >> owner >> x >> y >> param1 >> param2; cin.ignore();
            //cerr << "Entry: " << entityType << " " << owner << " " << x << " " << y << " " << param1 << " " << param2 << endl;

            switch( entityType )
            {
            case Entities::character:
                if( owner == myId )
                {
                    Character::get().my_pos = Position(y, x);
                    Field::get().set_character_pos( Character::get().my_pos );
                }
                break;
            case Entities::bomb:
                Field::get().set_bomb( {y, x}, param2 - 1 );
                break;
            case Entities::item:
                if( param1 == 1 )
                {
                    Field::get().set_range_upgrade( Position(y, x) );
                }
                else if( param1 == 2 )
                {
                    Field::get().set_count_upgrade( Position(y, x) );
                }
                break;
            default:
                break;
            }
        }

        cerr << Field::get().print() << endl;
        Character::get().check_pickups();
        Character::get().bomb_and_move();
    }
}