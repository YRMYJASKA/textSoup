/*
*    TextSoup, Yet another text editor
*    Copyright (C) 2017  Jyry Hjelt
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*/
// main.cpp

// Include the libraries
#include <iostream>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "files.hpp"
#include "logging.hpp"
#include "main.h"

using namespace std;

// Important variables
unsigned int MAX_X = 0, MAX_Y = 0; // Window's current dimensions
unsigned int CURS_X = 0, CURS_Y = 0; // Cursor's position
int key = 0; // The value of the key presses is stored into 'int key'

string fileName = ""; // Name of the file
vector<string> LineBuffer(1); // the buffer that stores the lines
bool running = true; // Boolean to determine if the program is running
unsigned int lineArea = 0; // Used to declare the area to draw the lines in

string location; // TextSoup's direcotry location

string messageBar = "";
MsgBarStatus MessageBarStatus = CLEAR;

int main(int count, char* option[])
{
    // Get the location of the source code
    getLocation();

    // cout << location << endl;
    Logging::logEntry("TextSoup starting up!", Logging::INFO);

    // Add the cursor buffer to the first line
    LineBuffer[0] = " ";

    // If there was an file name inputted
    if (count > 1) {
        // There must be a better way to write this
        if (!strcmp(option[1], "--version")) {
            cout << "Current version of TextSoup is v1.2.5" << endl;
            exit(EXIT_SUCCESS);
        } else if (!strcmp(option[1], "--help")) {
            printFile(location + "/info/help.txt");
            exit(EXIT_SUCCESS);
        } else if (!strcmp(option[1], "--license")) {
            printFile(location + "/LICENSE");
            exit(EXIT_SUCCESS);
        } else {
            fileName = option[1];
        }
    } else {
        fileName = "";
    }

    if (fileExists(fileName)) {
        LineBuffer = getFileLines(fileName);
        // Log the event
        Logging::logEntry("Loaded file (" + fileName + ")\n \t\t\t Lines: " + to_string(LineBuffer.size()),
            Logging::INFO);

        // If the file is empty add a line to prevent segFaults
        if (LineBuffer.size() < 1) {
            LineBuffer.push_back(" ");
        }
    }

    Logging::logEntry("Initializing ncurses...", Logging::INFO);

    // Initializing ncurses...
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(FALSE);
    start_color();

    // Initialize colour pairs for different colours
    init_pair(1, COLOR_BLACK, COLOR_WHITE); // Inverted colour pair (cursor)

    // Main loop
    while (running) {
        // Update
        updateScr();

        // If there is MessageBarStatus to handle (eg. save, exit)
        if (MessageBarStatus != CLEAR) {
            handleMsgBar(MessageBarStatus);
        } else {
            // If no MessageBarStatus to handle carry on business as usual

            // Fetch keypress
            key = getch();

            // Process the keypress...
            switch (key) {
            // Exit (^Q)
            case Q:
                MessageBarStatus = EXIT;
                break;
            // Find (^F)
            case F:
                MessageBarStatus = FIND;
                break;
            // Save (^S)
            case S:
                MessageBarStatus = SAVE;
                break;

            // Backspace
            case 127:
            case KEY_BACKSPACE:
                // if the cursor is at the start of a line
                if (CURS_X > 0) {
                    // Delete the character before the cursor
                    LineBuffer[CURS_Y].erase(CURS_X - 1, 1);
                    CURS_X--;
                } else {
                    // Delete the line and change the one above the cursor
                    if (CURS_Y > 0) {
                        LineBuffer[CURS_Y - 1].pop_back();
                        CURS_X = LineBuffer[CURS_Y - 1].length();
                        LineBuffer[CURS_Y - 1] += LineBuffer[CURS_Y];
                        LineBuffer.erase(LineBuffer.begin() + CURS_Y);
                        CURS_Y--; // Change to the line above

                        if (CURS_Y < lineArea && lineArea > 0)
                            lineArea--;
                    }
                }
                break;

            // Enter
            case ENTER:
                // Add the text on te rights side of the cursor
                // to the new line below
                LineBuffer.insert(
                    LineBuffer.begin() + CURS_Y + 1,
                    LineBuffer[CURS_Y].substr(CURS_X, LineBuffer[CURS_Y].length()));

                // Erase the right side from the previous line
                LineBuffer[CURS_Y].erase(CURS_X, LineBuffer[CURS_Y].length() - 1);

                // If the last line didnt have a cursor buffer
                if (LineBuffer[CURS_Y].back() != ' ') {
                    LineBuffer[CURS_Y] += ' '; // Add the cursor buffer to line
                }

                // Set correct  Y and X values
                CURS_Y++;
                if (CURS_Y >= MAX_Y - TOP_PADDING + lineArea) {
                    lineArea++;
                }
                CURS_X = 0;

                // Auto Indentation
                for (int i = 0; i < spacesLastLine(CURS_Y); i++) {
                    LineBuffer[CURS_Y].insert(LineBuffer[CURS_Y].begin() + CURS_X,
                        char(' '));
                    CURS_X += 1;
                }

                break;
            // Open a file
            case O:
                MessageBarStatus = OPEN;
                break;
            // Arrow keys
            case KEY_LEFT:
                if (CURS_X != 0) {
                    CURS_X--;
                }
                break;
            case KEY_RIGHT:
                if (CURS_X < LineBuffer[CURS_Y].length() - 1) {
                    CURS_X++;
                }
                break;
            case KEY_UP:
                if (CURS_Y != 0) {
                    CURS_Y--;
                    if (CURS_X + 1 >= LineBuffer[CURS_Y].length()) {
                        CURS_X = LineBuffer[CURS_Y].length() - 1;
                    }
                    if (CURS_Y < lineArea && lineArea > 0) {
                        lineArea--;
                    }
                }
                break;
            case KEY_DOWN:
                if (CURS_Y + 1 < LineBuffer.size()) {
                    CURS_Y++;
                    if (CURS_X + 1 >= LineBuffer[CURS_Y].length()) {
                        CURS_X = LineBuffer[CURS_Y].length() - 1;
                    }
                    if (CURS_Y >= MAX_Y - TOP_PADDING + lineArea) {
                        lineArea++;
                    }
                }
                break;

            // TAB key (WIP)
            case 9:
                for (int i = 0; i < 4; i++) {
                    LineBuffer[CURS_Y].insert(LineBuffer[CURS_Y].begin() + CURS_X,
                        char(' '));
                    CURS_X += 1;
                }
                break;

            // Add the keypress to the current line if a regular keypress
            default:
                LineBuffer[CURS_Y].insert(LineBuffer[CURS_Y].begin() + CURS_X,
                    char(key));
                CURS_X += 1;
                break;
            }
        }
        // Clear the screen to draw the screen correctly
        clear();
    }

    // Terminate the program
    endwin(); // End the ncurses session
    Logging::logEndSession(); // Send the end message to the log file
    return 0;
}
void updateScr()
{
    // Refresh
    refresh();
    getmaxyx(stdscr, MAX_Y, MAX_X);

    // Print status bar
    attron(COLOR_PAIR(1));
    mvprintw(0, 0, fileName.c_str());
    mvprintw(0, fileName.length(), " %i,%i L: %i", CURS_X, CURS_Y,
        LineBuffer.size());
    // Message bar (for various uses)
    mvprintw(1, 0, messageBar.c_str());
    attroff(COLOR_PAIR(1));

    // Horizontal line separating the main and top fields
    mvprintw(2, 0, "");
    for (unsigned int i = 0; i < MAX_X; i++) {
        addch(ACS_HLINE);
    }
    int z = 0; // A variable to keep track of where to print the lines
    for (unsigned int i = 0; i < LineBuffer.size(); i++) {
        // If the line is not in the specified area, skip it
        if (i < lineArea) {
            continue;
        }

        // If it is, draw it
        mvprintw(z + TOP_PADDING, 0, "%d", i + 1);
        if (i == CURS_Y) {
            // If this line has the cursor on it draw it char-by-char
            for (unsigned int x = 0; x < LineBuffer[i].length(); x++) {
                if (x == CURS_X) {
                    // Draw the cursor correctly
                    attron(COLOR_PAIR(1));
                    mvprintw(z + TOP_PADDING, x + LEFT_PADDING, "%c",
                        LineBuffer[i].at(x));
                    attroff(COLOR_PAIR(1));
                } else {
                    // Draw the regular character
                    mvprintw(z + TOP_PADDING, x + LEFT_PADDING, "%c",
                        LineBuffer[i].at(x));
                }
            }
        } else {
            // If not the cursor line draw without special handling
            mvprintw(z + TOP_PADDING, LEFT_PADDING, LineBuffer[i].c_str());
        }
        z++; // increment the position of lines
    }
}

// Get the location of the textSoup source directory
void getLocation()
{
    // File that contains the absolute path to the source directory
    ifstream iFILE("/etc/textSoup/location");
    Logging::logEntry("Trying to load location from /etc/textSoup/location",
        Logging::INFO);

    // Check file's conditions
    if (iFILE.good()) {
        getline(iFILE, location);
        if (location == "") {
            Logging::logEntry(
                "Empty location in /etc/textSoup/location! Check installation",
                Logging::FATAL);
            cout << "Empty location! Check Logs!" << endl;
            exit(EXIT_FAILURE);
        }
        Logging::logEntry("Location: " + location, Logging::INFO);
    } else {
        // Panic and quit the program
        cout << "No file found in /etc/textSoup!" << endl;
        cout << "Exiting....." << endl;
        cout << "Make sure you followed the installation correctly!" << endl;
        exit(EXIT_FAILURE);
    }

    iFILE.close(); // Close the file stream
}
// Handle the message bar's status
void handleMsgBar(MsgBarStatus status)
{
    switch (status) {
    // Save to a file
    case SAVE: {
        // initalize the message bar
        messageBar = "File name: " + fileName;

        updateScr();

        // Sub routine loop
        bool subRunning = true;
        string fileNameBuffer = fileName;
        while (subRunning) {
            updateScr();
            key = getch();
            switch (key) {
            // Backspace
            case 127:
            case KEY_BACKSPACE:
                // Delete the last character of the file name buffer
                if (fileNameBuffer.length() > 0) {
                    fileNameBuffer.pop_back();
                }
                break;
            // Quit dialog (^Q)
            case C:
                subRunning = false;
                break;
            // Enter
            case ENTER:
                // set the file name to be the filename buffer
                fileName = fileNameBuffer;
                // Save the text to the given file name
                writeToFile(fileName, LineBuffer);
                subRunning = false;
                break;
            default:
                fileNameBuffer += key;
            }
            messageBar = "File name: " + fileNameBuffer;
            clear();
        }

        // Reset the message bar
        messageBar = "";
        MessageBarStatus = CLEAR;
        break;
    }
    // Open a file by certain name
    case OPEN: {
        // Initialize the screen and messagebar
        messageBar = "File to open: ";
        updateScr();

        bool subRunning = true;
        string fileNameBuffer = "";
        while (subRunning) {
            updateScr();
            key = getch();
            switch (key) {
            // Backspace
            case 127:
            case KEY_BACKSPACE:
                // Delete the last character of the file name buffer
                if (fileNameBuffer.length() > 0) {
                    fileNameBuffer.pop_back();
                }
                break;
            // Quit dialog (^Q)
            case C:
                subRunning = false;
                break;
            // Enter
            case ENTER:
                fileName = fileNameBuffer;

                // Open the file
                LineBuffer = getFileLines(fileName);

                // Log the event
                Logging::logEntry("Loaded file (" + fileName + ")\n \t\t\t Lines: " + to_string(LineBuffer.size()),
                    Logging::INFO);
                subRunning = false;
                break;
            default:
                fileNameBuffer += key;
            }
            messageBar = "File name: " + fileNameBuffer;
            clear();
        }
        // Reset the message bar
        messageBar = "";
        MessageBarStatus = CLEAR;
        break;
    }
    case EXIT: {
        if (LineBuffer != getFileLines(fileName)) {
            bool subRunning = true;
            messageBar = "Save changes before you exit? (Y/n)";
            updateScr();
            while (subRunning) {
                key = getch();
                switch (key) {
                case 121:
                case ENTER:
                    // Print the lineBuffer into the file
                    // before exiting if 'y' or enter is pressed
                    if (fileName != "") {
                        writeToFile(fileName, LineBuffer);
                    } else {
                        handleMsgBar(SAVE);
                    }
                    subRunning = false;
                    running = false;
                    break;
                case Q:
                case 110:
                    // if 'n' or ^Q is pressed don't save before exit
                    subRunning = false;
                    running = false;
                    break;
                case C:
                    // Not working for some reason
                    // ^C  was pressed which means cancel the dialog
                    subRunning = false;
                    break;
                }
            }
        } else {
            running = false;
        }

        messageBar = "";
        MessageBarStatus = CLEAR;
        break;
    }
    case FIND: {
        // TODO: make it better, in every way
        messageBar = "Find?: ";
        updateScr();

        // Local varibales
        bool subRunning = true;
        string stringToFind = "";
        int currentHit = 0;

        // Save the last start position of the cursor in case of cancel
        int StartX = CURS_X;
        int StartY = CURS_Y;

        // Sub-routine for the find functionality
        while (subRunning) {
            updateScr();
            key = getch();
            switch (key) {
            // Backspace
            case 127:
            case KEY_BACKSPACE:
                // Delete the last character of the file name buffer
                if (stringToFind.length() > 0) {
                    stringToFind.pop_back();
                }
                break;
            // Quit dialog (^Q)
            case C:
                subRunning = false;
                CURS_X = StartX;
                CURS_Y = StartY;
                break;
            // Enter
            case ENTER:
                // ????
                searchFile(stringToFind);
                for (int i = 0; i < int(searchResults.size()); i++) {
                    cout << searchResults[i][0] << ", " << searchResults[i][1]
                         << endl;
                }
                cout << "------" << endl;
                // subRunning = false;
                break;
            // Increment the current search hit by 1
            case KEY_DOWN:
            case KEY_RIGHT:
                if (currentHit < int(searchResults.size() - 1)) {
                    currentHit++;
                }
                break;
            // Decrease the current search hit by 1
            case KEY_UP:
            case KEY_LEFT:
                if (currentHit > 0) {
                    currentHit--;
                }
                break;
            default:
                stringToFind += key;
            }

            messageBar = "Find?: " + stringToFind;

            searchFile(stringToFind);
            // if (searchResults.size() > 0) {
            CURS_X = searchResults[currentHit][0];
            CURS_Y = searchResults[currentHit][1];
            //}
            clear();
        }
        // Reset the message bar
        messageBar = "";
        MessageBarStatus = CLEAR;
        break;
    }
    case CLEAR:
    default:
        // not supposed to happen. Invalid value
        break;
    }
}

// Returns how many spaces were in front of the last line
int spacesLastLine(int y)
{
    int counter = 0;
    string line = LineBuffer[y - 1].substr(0, LineBuffer[y].length());

    // Iterate over the string (disregarding the space buffer)
    for (unsigned int i = 0; i < line.length() - 1; i++) {
        if (line.at(i) == ' ') {
            counter++;
        } else {
            break;
        }
    }

    return counter;
}

// Search a string in file and return results to variable searchResults
// (vector<vector<int>>)
void searchFile(string s)
{
    vector<int> buff;
    size_t found;

    searchResults.clear();

    for (unsigned int i = 0; i < LineBuffer.size(); i++) {
        found = LineBuffer[i].find(s);
        if (found != string::npos) {
            // TODO: Add the x and y values of the found sting into searchResults
            buff.push_back(int(found));
            buff.push_back(i);

            searchResults.push_back(buff);
        }
    }
}
