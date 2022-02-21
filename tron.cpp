#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <iostream>
#include <thread>
#include <string>
#include <filesystem>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define MAP_X 80
#define MAP_Y 21

bool is_alive = true;

struct player {
    int x_pos;
    int y_pos;
    char symbol;
    // 0 = up, 1 = down, 2 = left, 3 = right
    int direction;
    int score;
    int invincible;
    int teleport;

    int life_timer;
};

struct map {
    char map[MAP_Y][MAP_X];
    std::string winner = "";
    int highscore;
};

// Checks for if a key has been pressed
int kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

// Player one movements
void do_moves_one (map *m, player *one, const char k) {
    switch (k) {
        case 'w':
            if (one->direction != 1) {
            one->direction = 0;
            }
            break;
        case 's':
            if (one->direction != 0) {
            one->direction = 1;
            }
            break;
        case 'a':
            if (one->direction != 3) {
            one->direction = 2;
            }
            break;
        case 'd':
            if (one->direction != 2) {
            one->direction = 3;
            }
            break;
        case 27:
            is_alive = 0;
            m->winner = "No one";
            break;
    }
}

// Player two movements
void do_moves_two (map *m, player *two, const char k) {
    switch (k) {
        case 'i':
            if (two->direction != 1) {
            two->direction = 0;
            }
            break;
        case 'k':
            if (two->direction != 0) {
            two->direction = 1;
            }
            break;
        case 'j':
            if (two->direction != 3) {
            two->direction = 2;
            }
            break;
        case 'l':
            if (two->direction != 2) {
            two->direction = 3;
            }
            break;
    }
}

// Renders the map
void render_map(map *m, player *one, player *two) {
    for (int y = 0; y < MAP_Y; y++) {
        for (int x = 0; x < MAP_X; x++) {
            if (y == one->y_pos && x == one->x_pos) {
                mvaddch(y, x, one->symbol);
            }
            else if (y == two->y_pos && x == two->x_pos) {
                mvaddch(y, x, two->symbol);
            }
            else {
                mvaddch(y, x, m->map[y][x]);
            }
        }
    }
}

void add_powerups(map *m) {
    int count = 0;
    for (int y = 0; y < MAP_Y; y++) {
        for (int x = 0; x < MAP_X; x++) {
            if (m->map[y][x] == '!' || m->map[y][x] == '*') {
                count++;
            }
        }
    }

    if (count < 5) {

    char invincibility = '*';
    char teleport = '!';

    srand(time(0));
    int ran;

    ran = (rand() % 99) + 1;

    if (ran <= 10) {
    for (int i = 0; i < 1; i++) {
        int x = (rand() % 78) + 1;
        int y = (rand() % 19) + 1;

        if (m->map[y][x] == ' ') {
            m->map[y][x] = invincibility;
        }
        else {
            i--;
        }
    }
    } else {
    for (int s = 0; s < 1; s++) {
        int x = (rand() % 72) + 1;
        int y = (rand() % 14) + 1;

        if (m->map[y][x] == ' ') {
            m->map[y][x] = teleport;
        }
        else {
            s--;
        }
    }
    }

    }
}

void alt_add_powerups(map *m) {
    int count = 0;
    for (int y = 0; y < MAP_Y; y++) {
        for (int x = 0; x < MAP_X; x++) {
            if (m->map[y][x] == '*') {
                count++;
            }
        }
    }

    char invincibility = '*';

    srand(time(0));

    if (count < 1) {
    for (int i = 0; i < 1; i++) {
        int x = (rand() % 78) + 1;
        int y = (rand() % 19) + 1;

        if (m->map[y][x] == ' ') {
            m->map[y][x] = invincibility;
        }
        else {
            i--;
        }
    }
    }
}

// Reads highscore from ~/.tron/tron_highscore.txt and sets m->highscore equal to that
void init_highscore (map *m) {
    std::string home = getenv("HOME");
    std::string save_dir = home + "/.tron";

    DIR *dir = opendir(save_dir.c_str());

    if (!dir) {
        int check = mkdir(save_dir.c_str(), 0777);

        if (!check) {
            printf("Directory created.");
        }
        else {
            printf("Directory not created.");
        }
    }

    std::string filename = save_dir + "/tron_highscore.txt";
    FILE *f = fopen(filename.c_str(), "a+");

    fscanf(f, "%d", &m->highscore);

    fclose(f);
    closedir(dir);
}

// Saves new highscore to ~/.tron/tron_highscore.txt
void save_new_highscore (map *m) {
    std::string home = getenv("HOME");
    std::string save_dir = home + "/.tron";

    DIR *dir = opendir(save_dir.c_str());

    std::string filename = save_dir + "/tron_highscore.txt";
    FILE *f = fopen(filename.c_str(), "w+");

    fprintf(f, "%d", m->highscore);

    fclose(f);
    closedir(dir);
}

int main (int argc, char *argv[]) {
    map m;
    int timer = 3;
    char k;

    if (argc == 1 || (argc > 1 && argv[1][2] == '1')) {
    // Read highscore from file if exists
    init_highscore(&m);

    initscr();
    raw();
    noecho();
    curs_set(0);

    // Fill in map
    for (int y = 0; y < MAP_Y; y++) {
        for (int x = 0; x < MAP_X; x++) {
            if (x == 0 || x == 79) {
                m.map[y][x] = '|';
            }
            else if (y == 0 || y == 20) {
                m.map[y][x] = '-';
            }
            else {
                m.map[y][x] = ' ';
            }
        }
    }
    
    player one;
    player two;
    one.x_pos = 20;
    one.y_pos = 10;
    one.symbol = '&';
    two.x_pos = 60;
    two.y_pos = 10;
    two.symbol = '@';
    
    one.direction = 0;
    two.direction = 0;
    one.score = 0;
    two.score = 0;

    one.teleport = 5;
    two.teleport = 5;
    one.invincible = 0;
    two.invincible = 0;

    render_map(&m, &one, &two);

    refresh();

    // Timer at the beginning of a match
    while (timer > 0) {
        mvprintw(15, 35, "Highscore: %d", m.highscore);

        // Display numbers
        mvprintw(8, 36, "            ");
        mvprintw(9, 36, "            ");
        mvprintw(10, 36, "            ");
        mvprintw(11, 36, "            ");
        mvprintw(12, 36, "            ");
        mvprintw(13, 36, "            ");
        if (timer == 1) {
            mvprintw(8, 38, " ____ ");
            mvprintw(9, 38, "/_   |");
            mvprintw(10, 38, " |   |");
            mvprintw(11, 38, " |   |");
            mvprintw(12, 38, " |___|");
        }
        if (timer == 2) {
            mvprintw(8, 36, " ________");
            mvprintw(9, 36, "\\_____   \\");
            mvprintw(10, 36, "  /  ____/");
            mvprintw(11, 36, " /       \\");
            mvprintw(12, 36, " \\_______ \\");
            mvprintw(13, 36, "         \\/");
        }
        if (timer == 3) {
            mvprintw(8, 36, " ________");
            mvprintw(9, 36, "\\_____   \\");
            mvprintw(10, 36, "  _(__   < ");
            mvprintw(11, 36, " /        \\");
            mvprintw(12, 36, "/______   /");
            mvprintw(13, 36, "       \\/ ");
        }

        // Display controls
        mvprintw(21, 0, "P1 Controls: w");
        mvprintw(22, 12, "asd");
        mvprintw(21, 60, "P2 Controls: i");
        mvprintw(22, 72, "jkl");

        refresh();
        usleep(1000000/1);
        timer--;
    }

    clear();

    int power_timer = 0;
    int one_invincible_timer = 20;
    int two_invincible_timer = 20;

    while (is_alive) {
        usleep(1000000/10);

        // Add powerups to the map. Max of 5;
        if (power_timer == 0) {
            add_powerups(&m);
            power_timer = 50;
        }

        power_timer--;

        if (kbhit()) {
            std::cin >> k;
            do_moves_one(&m, &one, k);
            std::thread th1(do_moves_two, &m, &two, k);
            th1.join();
        }

        if (one.invincible == 1) {
            if (one_invincible_timer == 0) {
                one.invincible = 0;
            }

            one_invincible_timer--;

            if (one.direction == 0)
            {
                one.y_pos -= 1;
            }
            if (one.direction == 1)
            {
                one.y_pos += 1;
            }
            if (one.direction == 2)
            {
                one.x_pos -= 1;
            }
            if (one.direction == 3)
            {
                one.x_pos += 1;
            }
        }
        else {

        // Move player 1
        if (one.direction == 0) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.y_pos -= 1;
        }
        if (one.direction == 1) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.y_pos += 1;
        }
        if (one.direction == 2) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.x_pos -= 1;
        }
        if (one.direction == 3) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.x_pos += 1;
        }

        }

        if (two.invincible == 1) {
            if (two_invincible_timer == 0) {
                two.invincible = 0;
            }

            two_invincible_timer--;

            if (two.direction == 0)
            {
                two.y_pos -= 1;
            }
            if (two.direction == 1)
            {
                two.y_pos += 1;
            }
            if (two.direction == 2)
            {
                two.x_pos -= 1;
            }
            if (two.direction == 3)
            {
                two.x_pos += 1;
            }
        }
        else {

        // Move player 2
        if (two.direction == 0) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.y_pos -= 1;
        }
        if (two.direction == 1) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.y_pos += 1;
        }
        if (two.direction == 2) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.x_pos -= 1;
        }
        if (two.direction == 3) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.x_pos += 1;
        }

        }

        // Check for Power Ups
        if (m.map[one.y_pos][one.x_pos] == '!') {
        if (one.direction == 0) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.y_pos -= 6;
        }
        if (one.direction == 1) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.y_pos += 6;
        }
        if (one.direction == 2) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.x_pos -= 6;
        }
        if (one.direction == 3) {
            m.map[one.y_pos][one.x_pos] = '#';
            one.x_pos += 6;
        }
        }
        else if (m.map[one.y_pos][one.x_pos] == '*') {
            m.map[one.y_pos][one.x_pos] = '#';
            one.invincible = 1;
            one_invincible_timer = 10;
        }

        if (m.map[two.y_pos][two.x_pos] == '!') {
        if (two.direction == 0) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.y_pos -= 6;
        }
        if (two.direction == 1) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.y_pos += 6;
        }
        if (two.direction == 2) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.x_pos -= 6;
        }
        if (two.direction == 3) {
            m.map[two.y_pos][two.x_pos] = '#';
            two.x_pos += 6;
        }
        }
        else if (m.map[two.y_pos][two.x_pos] == '*') {
            m.map[two.y_pos][two.x_pos] = '#';
            two.invincible = 1;
            two_invincible_timer = 10;
        }

        // Calculate scores: 10 points per move for being next to a wall, one point otherwise
        one.score++;
        two.score++;

        if (one.direction == 0) { // up
        if (m.map[one.y_pos][one.x_pos+1] == '#' || m.map[one.y_pos][one.x_pos-1] == '#'
            || m.map[one.y_pos-1][one.x_pos] == '#'
            || m.map[one.y_pos+1][one.x_pos+1] == '#' || m.map[one.y_pos-1][one.x_pos-1] == '#'
            || m.map[one.y_pos-1][one.x_pos+1] == '#' || m.map[one.y_pos+1][one.x_pos-1] == '#'
            || m.map[one.y_pos][one.x_pos+1] == '|' || m.map[one.y_pos][one.x_pos-1] == '|'
            || m.map[one.y_pos+1][one.x_pos] == '|' || m.map[one.y_pos-1][one.x_pos] == '|'
            || m.map[one.y_pos+1][one.x_pos+1] == '|' || m.map[one.y_pos-1][one.x_pos-1] == '|'
            || m.map[one.y_pos-1][one.x_pos+1] == '|' || m.map[one.y_pos+1][one.x_pos-1] == '|'
            || m.map[one.y_pos][one.x_pos+1] == '-' || m.map[one.y_pos][one.x_pos-1] == '-'
            || m.map[one.y_pos+1][one.x_pos] == '-' || m.map[one.y_pos-1][one.x_pos] == '-'
            || m.map[one.y_pos+1][one.x_pos+1] == '-' || m.map[one.y_pos-1][one.x_pos-1] == '-'
            || m.map[one.y_pos-1][one.x_pos+1] == '-' || m.map[one.y_pos+1][one.x_pos-1] == '-') {
                one.score += 9;
        }
        }
        else if (one.direction == 1) { // down
        if (m.map[one.y_pos][one.x_pos+1] == '#' || m.map[one.y_pos][one.x_pos-1] == '#'
            || m.map[one.y_pos+1][one.x_pos] == '#'
            || m.map[one.y_pos+1][one.x_pos+1] == '#' || m.map[one.y_pos-1][one.x_pos-1] == '#'
            || m.map[one.y_pos-1][one.x_pos+1] == '#' || m.map[one.y_pos+1][one.x_pos-1] == '#'
            || m.map[one.y_pos][one.x_pos+1] == '|' || m.map[one.y_pos][one.x_pos-1] == '|'
            || m.map[one.y_pos+1][one.x_pos] == '|' || m.map[one.y_pos-1][one.x_pos] == '|'
            || m.map[one.y_pos+1][one.x_pos+1] == '|' || m.map[one.y_pos-1][one.x_pos-1] == '|'
            || m.map[one.y_pos-1][one.x_pos+1] == '|' || m.map[one.y_pos+1][one.x_pos-1] == '|'
            || m.map[one.y_pos][one.x_pos+1] == '-' || m.map[one.y_pos][one.x_pos-1] == '-'
            || m.map[one.y_pos+1][one.x_pos] == '-' || m.map[one.y_pos-1][one.x_pos] == '-'
            || m.map[one.y_pos+1][one.x_pos+1] == '-' || m.map[one.y_pos-1][one.x_pos-1] == '-'
            || m.map[one.y_pos-1][one.x_pos+1] == '-' || m.map[one.y_pos+1][one.x_pos-1] == '-') {
                one.score += 9;
        }
        }
        else if (one.direction == 2) { // left
        if (m.map[one.y_pos][one.x_pos-1] == '#'
            || m.map[one.y_pos+1][one.x_pos] == '#' || m.map[one.y_pos-1][one.x_pos] == '#'
            || m.map[one.y_pos+1][one.x_pos+1] == '#' || m.map[one.y_pos-1][one.x_pos-1] == '#'
            || m.map[one.y_pos-1][one.x_pos+1] == '#' || m.map[one.y_pos+1][one.x_pos-1] == '#'
            || m.map[one.y_pos][one.x_pos+1] == '|' || m.map[one.y_pos][one.x_pos-1] == '|'
            || m.map[one.y_pos+1][one.x_pos] == '|' || m.map[one.y_pos-1][one.x_pos] == '|'
            || m.map[one.y_pos+1][one.x_pos+1] == '|' || m.map[one.y_pos-1][one.x_pos-1] == '|'
            || m.map[one.y_pos-1][one.x_pos+1] == '|' || m.map[one.y_pos+1][one.x_pos-1] == '|'
            || m.map[one.y_pos][one.x_pos+1] == '-' || m.map[one.y_pos][one.x_pos-1] == '-'
            || m.map[one.y_pos+1][one.x_pos] == '-' || m.map[one.y_pos-1][one.x_pos] == '-'
            || m.map[one.y_pos+1][one.x_pos+1] == '-' || m.map[one.y_pos-1][one.x_pos-1] == '-'
            || m.map[one.y_pos-1][one.x_pos+1] == '-' || m.map[one.y_pos+1][one.x_pos-1] == '-') {
                one.score += 4;
        }
        }
        else if (one.direction == 3) { // right
        if (m.map[one.y_pos][one.x_pos+1] == '#'
            || m.map[one.y_pos+1][one.x_pos] == '#' || m.map[one.y_pos-1][one.x_pos] == '#'
            || m.map[one.y_pos+1][one.x_pos+1] == '#' || m.map[one.y_pos-1][one.x_pos-1] == '#'
            || m.map[one.y_pos-1][one.x_pos+1] == '#' || m.map[one.y_pos+1][one.x_pos-1] == '#'
            || m.map[one.y_pos][one.x_pos+1] == '|' || m.map[one.y_pos][one.x_pos-1] == '|'
            || m.map[one.y_pos+1][one.x_pos] == '|' || m.map[one.y_pos-1][one.x_pos] == '|'
            || m.map[one.y_pos+1][one.x_pos+1] == '|' || m.map[one.y_pos-1][one.x_pos-1] == '|'
            || m.map[one.y_pos-1][one.x_pos+1] == '|' || m.map[one.y_pos+1][one.x_pos-1] == '|'
            || m.map[one.y_pos][one.x_pos+1] == '-' || m.map[one.y_pos][one.x_pos-1] == '-'
            || m.map[one.y_pos+1][one.x_pos] == '-' || m.map[one.y_pos-1][one.x_pos] == '-'
            || m.map[one.y_pos+1][one.x_pos+1] == '-' || m.map[one.y_pos-1][one.x_pos-1] == '-'
            || m.map[one.y_pos-1][one.x_pos+1] == '-' || m.map[one.y_pos+1][one.x_pos-1] == '-') {
                one.score += 4;
        }
        }

        if (two.direction == 0) { // up
        if (m.map[two.y_pos][two.x_pos+1] == '#' || m.map[two.y_pos][two.x_pos-1] == '#'
            || m.map[two.y_pos-1][two.x_pos] == '#'
            || m.map[two.y_pos+1][two.x_pos+1] == '#' || m.map[two.y_pos-1][two.x_pos-1] == '#'
            || m.map[two.y_pos-1][two.x_pos+1] == '#' || m.map[two.y_pos+1][two.x_pos-1] == '#'
            || m.map[two.y_pos][two.x_pos+1] == '|' || m.map[two.y_pos][two.x_pos-1] == '|'
            || m.map[two.y_pos+1][two.x_pos] == '|' || m.map[two.y_pos-1][two.x_pos] == '|'
            || m.map[two.y_pos+1][two.x_pos+1] == '|' || m.map[two.y_pos-1][two.x_pos-1] == '|'
            || m.map[two.y_pos-1][two.x_pos+1] == '|' || m.map[two.y_pos+1][two.x_pos-1] == '|'
            || m.map[two.y_pos][two.x_pos+1] == '-' || m.map[two.y_pos][two.x_pos-1] == '-'
            || m.map[two.y_pos+1][two.x_pos] == '-' || m.map[two.y_pos-1][two.x_pos] == '-'
            || m.map[two.y_pos+1][two.x_pos+1] == '-' || m.map[two.y_pos-1][two.x_pos-1] == '-'
            || m.map[two.y_pos-1][two.x_pos+1] == '-' || m.map[two.y_pos+1][two.x_pos-1] == '-') {
                two.score += 4;
        }
        }
        else if (two.direction == 1) { // down
        if (m.map[two.y_pos][two.x_pos+1] == '#' || m.map[two.y_pos][two.x_pos-1] == '#'
            || m.map[two.y_pos+1][two.x_pos] == '#'
            || m.map[two.y_pos+1][two.x_pos+1] == '#' || m.map[two.y_pos-1][two.x_pos-1] == '#'
            || m.map[two.y_pos-1][two.x_pos+1] == '#' || m.map[two.y_pos+1][two.x_pos-1] == '#'
            || m.map[two.y_pos][two.x_pos+1] == '|' || m.map[two.y_pos][two.x_pos-1] == '|'
            || m.map[two.y_pos+1][two.x_pos] == '|' || m.map[two.y_pos-1][two.x_pos] == '|'
            || m.map[two.y_pos+1][two.x_pos+1] == '|' || m.map[two.y_pos-1][two.x_pos-1] == '|'
            || m.map[two.y_pos-1][two.x_pos+1] == '|' || m.map[two.y_pos+1][two.x_pos-1] == '|'
            || m.map[two.y_pos][two.x_pos+1] == '-' || m.map[two.y_pos][two.x_pos-1] == '-'
            || m.map[two.y_pos+1][two.x_pos] == '-' || m.map[two.y_pos-1][two.x_pos] == '-'
            || m.map[two.y_pos+1][two.x_pos+1] == '-' || m.map[two.y_pos-1][two.x_pos-1] == '-'
            || m.map[two.y_pos-1][two.x_pos+1] == '-' || m.map[two.y_pos+1][two.x_pos-1] == '-') {
                two.score += 4;
        }
        }
        else if (two.direction == 2) { // left
        if (m.map[two.y_pos][two.x_pos-1] == '#'
            || m.map[two.y_pos+1][two.x_pos] == '#' || m.map[two.y_pos-1][two.x_pos] == '#'
            || m.map[two.y_pos+1][two.x_pos+1] == '#' || m.map[two.y_pos-1][two.x_pos-1] == '#'
            || m.map[two.y_pos-1][two.x_pos+1] == '#' || m.map[two.y_pos+1][two.x_pos-1] == '#'
            || m.map[two.y_pos][two.x_pos+1] == '|' || m.map[two.y_pos][two.x_pos-1] == '|'
            || m.map[two.y_pos+1][two.x_pos] == '|' || m.map[two.y_pos-1][two.x_pos] == '|'
            || m.map[two.y_pos+1][two.x_pos+1] == '|' || m.map[two.y_pos-1][two.x_pos-1] == '|'
            || m.map[two.y_pos-1][two.x_pos+1] == '|' || m.map[two.y_pos+1][two.x_pos-1] == '|'
            || m.map[two.y_pos][two.x_pos+1] == '-' || m.map[two.y_pos][two.x_pos-1] == '-'
            || m.map[two.y_pos+1][two.x_pos] == '-' || m.map[two.y_pos-1][two.x_pos] == '-'
            || m.map[two.y_pos+1][two.x_pos+1] == '-' || m.map[two.y_pos-1][two.x_pos-1] == '-'
            || m.map[two.y_pos-1][two.x_pos+1] == '-' || m.map[two.y_pos+1][two.x_pos-1] == '-') {
                two.score += 4;
        }
        }
        else if (two.direction == 3) { // right
        if (m.map[two.y_pos][two.x_pos+1] == '#'
            || m.map[two.y_pos+1][two.x_pos] == '#' || m.map[two.y_pos-1][two.x_pos] == '#'
            || m.map[two.y_pos+1][two.x_pos+1] == '#' || m.map[two.y_pos-1][two.x_pos-1] == '#'
            || m.map[two.y_pos-1][two.x_pos+1] == '#' || m.map[two.y_pos+1][two.x_pos-1] == '#'
            || m.map[two.y_pos][two.x_pos+1] == '|' || m.map[two.y_pos][two.x_pos-1] == '|'
            || m.map[two.y_pos+1][two.x_pos] == '|' || m.map[two.y_pos-1][two.x_pos] == '|'
            || m.map[two.y_pos+1][two.x_pos+1] == '|' || m.map[two.y_pos-1][two.x_pos-1] == '|'
            || m.map[two.y_pos-1][two.x_pos+1] == '|' || m.map[two.y_pos+1][two.x_pos-1] == '|'
            || m.map[two.y_pos][two.x_pos+1] == '-' || m.map[two.y_pos][two.x_pos-1] == '-'
            || m.map[two.y_pos+1][two.x_pos] == '-' || m.map[two.y_pos-1][two.x_pos] == '-'
            || m.map[two.y_pos+1][two.x_pos+1] == '-' || m.map[two.y_pos-1][two.x_pos-1] == '-'
            || m.map[two.y_pos-1][two.x_pos+1] == '-' || m.map[two.y_pos+1][two.x_pos-1] == '-') {
                two.score += 4;
        }
        }

        // Check if player is alive or not. 500 points for lasting longer than the other player.
        if (two.x_pos <= 0 || two.x_pos >= 79 || two.y_pos <= 0 || two.y_pos >= 20) {
            is_alive = 0;
            one.score += 500;
            m.winner = "Player One";
        }

        if (m.map[two.y_pos][two.x_pos] == '#') {
            if (two.invincible == 0) {
                is_alive = 0;
                one.score += 500;
                m.winner = "Player One";
            }
        }

        if (one.x_pos <= 0|| one.x_pos >= 79 || one.y_pos <= 0 || one.y_pos >= 20) {
            is_alive = 0;
            two.score += 500;
            m.winner = "Player Two";
        }

        if (m.map[one.y_pos][one.x_pos] == '#') {
            if (one.invincible == 0) {
                is_alive = 0;
                two.score += 500;
                m.winner = "Player Two";
            }
        }

        if (is_alive == 0) {
            break;
        }

        render_map(&m, &one, &two);
        mvprintw(21, 0, "P1 Score: %d", one.score);
        mvprintw(21, 65, "P2 Score: %d", two.score);

        refresh();
    }

    // If a player score is greater than the highscore, update the highscore
    if (one.score > m.highscore || two.score > m.highscore) {
        m.highscore = (one.score > two.score) ? one.score : two.score;
        save_new_highscore(&m);
    }

    clear();

    mvprintw(8, 28, "%s is the winner!", m.winner.c_str());
    mvprintw(10, 36, "P1: %d", one.score);
    mvprintw(11, 36, "P2: %d", two.score);
    mvprintw (13, 30, "Highscore: %d", m.highscore);
    refresh();

    usleep(1000000/0.5);
    endwin();

    std::cout << m.winner + " is the winner!\n";
    printf("P1 Score: %d\n", one.score);
    printf("P2 Score: %d\n", two.score);
    }

    if (argc > 1 && argv[1][2] == '2') {
    initscr();
    raw();
    noecho();
    curs_set(0);

    // Fill in map
    for (int y = 0; y < MAP_Y; y++) {
        for (int x = 0; x < MAP_X; x++) {
            if (x == 0 || x == 79) {
                m.map[y][x] = '|';
            }
            else if (y == 0 || y == 20) {
                m.map[y][x] = '-';
            }
            else {
                m.map[y][x] = ' ';
            }
        }
    }
    
    player one;
    player two;
    one.x_pos = 20;
    one.y_pos = 10;
    one.symbol = '&';
    two.x_pos = 60;
    two.y_pos = 10;
    two.symbol = '@';
    
    one.direction = 0;
    two.direction = 0;

    one.life_timer = 100;
    two.life_timer = 100;

    render_map(&m, &one, &two);

    refresh();

    // Timer at the beginning of a match
    while (timer > 0) {
        // Display numbers
        mvprintw(8, 36, "            ");
        mvprintw(9, 36, "            ");
        mvprintw(10, 36, "            ");
        mvprintw(11, 36, "            ");
        mvprintw(12, 36, "            ");
        mvprintw(13, 36, "            ");
        if (timer == 1) {
            mvprintw(8, 38, " ____ ");
            mvprintw(9, 38, "/_   |");
            mvprintw(10, 38, " |   |");
            mvprintw(11, 38, " |   |");
            mvprintw(12, 38, " |___|");
        }
        if (timer == 2) {
            mvprintw(8, 36, " ________");
            mvprintw(9, 36, "\\_____   \\");
            mvprintw(10, 36, "  /  ____/");
            mvprintw(11, 36, " /       \\");
            mvprintw(12, 36, " \\_______ \\");
            mvprintw(13, 36, "         \\/");
        }
        if (timer == 3) {
            mvprintw(8, 36, " ________");
            mvprintw(9, 36, "\\_____   \\");
            mvprintw(10, 36, "  _(__   < ");
            mvprintw(11, 36, " /        \\");
            mvprintw(12, 36, "/______   /");
            mvprintw(13, 36, "       \\/ ");
        }

        // Display controls
        mvprintw(21, 0, "P1 Controls: w");
        mvprintw(22, 12, "asd");
        mvprintw(21, 60, "P2 Controls: i");
        mvprintw(22, 72, "jkl");

        refresh();
        usleep(1000000/1);
        timer--;
    }

    clear();

    int power_timer = 0;

    while (is_alive) {
        usleep(1000000/10);

        // Add powerups to the map. Max of 5;
        if (power_timer == 0) {
            alt_add_powerups(&m);
            power_timer = 15;
        }

        power_timer--;

        if (kbhit()) {
            std::cin >> k;
            do_moves_one(&m, &one, k);
            std::thread th1(do_moves_two, &m, &two, k);
            th1.join();
        }

        one.life_timer--;
        two.life_timer--;
        
        // Move player 1
        if (one.direction == 0) {
            one.y_pos -= 1;
        }
        if (one.direction == 1) {
            one.y_pos += 1;
        }
        if (one.direction == 2) {
            one.x_pos -= 1;
        }
        if (one.direction == 3) {
            one.x_pos += 1;
        }

        // Move player 2
        if (two.direction == 0) {
            two.y_pos -= 1;
        }
        if (two.direction == 1) {
            two.y_pos += 1;
        }
        if (two.direction == 2) {
            two.x_pos -= 1;
        }
        if (two.direction == 3) {
            two.x_pos += 1;
        }

        // Check for time boost
        if (m.map[one.y_pos][one.x_pos] == '*') {
            m.map[one.y_pos][one.x_pos] = ' ';
            one.life_timer += 50;
        }

        if (m.map[two.y_pos][two.x_pos] == '*') {
            m.map[two.y_pos][two.x_pos] = ' ';
            two.life_timer += 50;
        }

        // Check if player is alive or not.
        if (two.x_pos == 0 || two.x_pos == 79 || two.y_pos == 0 || two.y_pos == 20
            || two.life_timer == 0) {
            is_alive = 0;
            m.winner = "Player One";
        }

        if (one.x_pos == 0|| one.x_pos == 79 || one.y_pos == 0 || one.y_pos == 20
            || one.life_timer == 0) {
            is_alive = 0;
            m.winner = "Player Two";
        }

        if (is_alive == 0) {
            break;
        }

        render_map(&m, &one, &two);
        mvprintw(21, 0, "P1 Life: %d   ", one.life_timer);
        mvprintw(21, 65, "P2 Life: %d   ", two.life_timer);

        refresh();
    }

    clear();

    mvprintw(8, 28, "%s is the winner!", m.winner.c_str());
    refresh();

    usleep(1000000/0.5);
    endwin();

    std::cout << m.winner + " is the winner!\n";
    }
}