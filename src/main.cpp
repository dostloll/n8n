#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <utility>

using namespace std;

void setNonBlocking(bool enable) {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    if (enable) {
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_cc[VMIN] = 1;
    } else {
        ttystate.c_lflag |= ICANON;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    fcntl(STDIN_FILENO, F_SETFL, enable ? O_NONBLOCK : 0);
}

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void clearScreen() {
    cout << "\033[2J\033[1;1H";
}

void draw(const vector<string>& screen) {
    clearScreen();
    for (const auto& line : screen) {
        cout << line << endl;
    }
}

int main() {
    setNonBlocking(true);
    const int width = 50;
    const int height = 10;
    vector<string> screen(height, string(width, ' '));
    int dinoY = height - 3;
    int dinoX = 5;
    bool jumping = false;
    int jumpHeight = 0;
    vector<pair<int, int>> obstacles;
    int score = 0;
    int speed = 100;

    while (true) {
        // Update dino
        if (jumping) {
            jumpHeight++;
            if (jumpHeight > 3) {
                jumping = false;
                jumpHeight = 0;
            }
        } else {
            dinoY = height - 3;
        }

        // Check input
        if (kbhit()) {
            char ch = getch();
            if (ch == ' ') {
                if (!jumping) {
                    jumping = true;
                    dinoY -= 2;
                }
            }
        }

        // Move obstacles
        for (auto& obs : obstacles) {
            obs.first--;
        }
        obstacles.erase(remove_if(obstacles.begin(), obstacles.end(), [](const pair<int, int>& p) { return p.first < 0; }), obstacles.end());

        // Add new obstacle
        if (rand() % 20 == 0) {
            obstacles.push_back({width - 1, height - 2});
        }

        // Draw ground
        for (int i = 0; i < width; i++) {
            screen[height - 1][i] = '-';
        }

        // Draw dino
        screen[dinoY][dinoX] = 'D';
        if (dinoY + 1 < height) screen[dinoY + 1][dinoX] = '|';
        if (dinoY + 2 < height) screen[dinoY + 2][dinoX] = '|';

        // Draw obstacles
        for (const auto& obs : obstacles) {
            if (obs.first >= 0 && obs.first < width) {
                screen[obs.second][obs.first] = '|';
            }
        }

        // Check collision
        for (const auto& obs : obstacles) {
            if (obs.first == dinoX && (obs.second == dinoY || obs.second == dinoY + 1 || obs.second == dinoY + 2)) {
                clearScreen();
                cout << "Game Over! Score: " << score << endl;
                setNonBlocking(false);
                return 0;
            }
        }

        // Draw score
        string scoreStr = "Score: " + to_string(score);
        for (size_t i = 0; i < scoreStr.size(); i++) {
            screen[0][i] = scoreStr[i];
        }

        draw(screen);

        // Clear screen for next frame
        for (auto& line : screen) {
            fill(line.begin(), line.end(), ' ');
        }

        score++;
        this_thread::sleep_for(chrono::milliseconds(speed));
    }

    setNonBlocking(false);
    return 0;
}