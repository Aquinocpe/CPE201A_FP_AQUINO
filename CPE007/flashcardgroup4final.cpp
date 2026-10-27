#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>     // _getch() for hidden password typing
#endif
using namespace std;
//THIS ONE IS JUST FOR OLD COMPILERS LIKE C++98
// ---- small helpers so the program also works on old C++ (C++98) compilers
string toStr(int n) {                       // same job as toStr()
    ostringstream out;
    out << n;
    return out.str();
}
//this converts text to numbers
bool parseInt(const string& s, int& out) {  // same job as stoi(), but never throws
    if (s.empty()) return false;
    char* end = 0;
    long v = strtol(s.c_str(), &end, 10);
    if (*end != '\0' || v < -2000000000L || v > 2000000000L) return false;
    out = (int)v; //exactly works just like stoi, you dont need to type "cout <<"
    return true;
}

// data
struct Question { 
    string text;
    string choices[4];   // A, B, C, D
    char answer;         // 'A' to 'D'
    int points;
    bool isBlank;           // true = fill in the blank, false = multiple choice (this will be for the professor)
    string blankAnswer;     // the correct answer for a fill in the blank (this is the string for the blank questions) -abi

    Question() : answer('A'), points(1), isBlank(false) {}
};

struct Test {
    string subject;      // one of the 4 subjects
    string topic;        // set by the professor (e.g. Chemistry)
    vector<Question> questions;
};

vector<Test> tests;

const int NUM_SUBJECTS = 4;
const string SUBJECTS[NUM_SUBJECTS] = { "Science", "Mathematics", "Computer", "English" };

// ACCOUNTS: Dito ginagawa ang sign up, login, at pag-manage ng users.
struct User {
    string username;
    string password;
    string role;          // "teacher" or "student"
};

vector<User> users; //this is a global list for all accounts
string currentUser = "";  // who is logged in right now
//now this part, this is responsible for where it creates txt files for the users and tests, including questions and answers 
const string USERS_FILE   = "users.txt";      // saved as: username password role
const string TESTS_FILE   = "tests.txt";      // all the tests and their questions
const string TEACHER_CODE = "TEACH2026";      // needed to sign up as a professor
const string BACK         = "/back";          // typed at text prompts to go back
const int    MAX_ATTEMPTS = 3;                // login attempts

// Comment this line out if you want passwords to show while typing
#define HIDE_PASSWORD_INPUT //this hide passwords with asterisk " * "

// colors 
string RESET  = "\033[0m";
string BOLD   = "\033[1m";
string CYAN   = "\033[36m";	//this is a collection of colors which we will used for the flashcard itself
string GREEN  = "\033[32m";
string RED    = "\033[31m";
string YELLOW = "\033[33m";

const int CARD_WIDTH = 52;   // this will be the  inner text width of a card

//  console helpers
void disableColors() {
    RESET = BOLD = CYAN = GREEN = RED = YELLOW = "";
}
//this part is for the colors of the flashcard-abraham
void setupConsole() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h == INVALID_HANDLE_VALUE || !GetConsoleMode(h, &mode) ||
        !SetConsoleMode(h, mode | 0x0004))
        disableColors();   // old console: plain text only
#endif
}

void clearScreen() { // this will clear the screen
#ifdef _WIN32
    system("cls"); //windows will run system "cls"
#else
    cout << "\033[2J\033[H";
#endif //then the other ones will print the escape code
}

string readLine(const string& prompt) { // this will read whole lines
    string s;
    cout << prompt;
    if (!getline(cin, s)) exit(0);   // input closed (Ctrl+Z / Ctrl+D)
    return s;
}
//these are our input checkers this will prevent the program from crashing -abi
void pauseScreen() {
    readLine("\nPress Enter to continue...");
}
//this will read the numbers/integers basically a true or false checker
int readInt(const string& prompt, int lo, int hi, bool allowBackWord = false) {
    while (true) {
        string s = readLine(prompt);
        if (allowBackWord && lo == 0 && s == BACK) return 0;   // typing /back = choosing 0
        int v;
        if (parseInt(s, v) && v >= lo && v <= hi) return v;
        cout << "Please enter a number from " << lo << " to " << hi << ".\n";
    }
}
//this part will only accept the choices A,B,C,D anything else will be denied 
char readChoiceLetter(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        if (s.size() == 1 && toupper(s[0]) >= 'A' && toupper(s[0]) <= 'D')
            return (char)toupper(s[0]);
        cout << "Please enter A, B, C, or D.\n";
    }
}
//for this one this will ask if the user would like to proceed, or confirm.
bool askYesNo(const string& prompt) {
    while (true) {
        string s = readLine(prompt + " (y/n): ");
        if (!s.empty() && (tolower(s[0]) == 'y')) return true;
        if (!s.empty() && (tolower(s[0]) == 'n')) return false;
    }
}

// flashcard UI for this program
vector<string> wrapText(const string& text, size_t width) {		
    vector<string> lines;
    istringstream words(text);
    string word, line;						// the size will adjust based on how much strings will be inside it
    while (words >> word) {
        if (!line.empty() && line.size() + word.size() + 1 > width) {
            lines.push_back(line);
            line.clear();
        }
        line += (line.empty() ? "" : " ") + word;
    }
    if (!line.empty()) lines.push_back(line); // this changes the size of the UI if theres blank spaces it overwrites it
    if (lines.empty()) lines.push_back("");
    return lines;
}

string repeatStr(const string& s, int n) {
    string out;
    for (int i = 0; i < n; i++) out += s;
    return out;
}

// Prints one row inside the card. align: 'c' = centered, 'l' = left
void cardRow(const string& content, char align, const string& color) {
    int w = CARD_WIDTH;
    int len = (int)content.size();
    if (len > w) len = w;
    int left = (align == 'c') ? (w - len) / 2 : 0;
    int right = w - len - left;
    cout << color << "| " << RESET
         << string(left, ' ') << content << string(right, ' ')
         << color << " |" << RESET << "\n";
}

void cardBlank(const string& color) {
    cardRow("", 'l', color);
}
//This is drawCard(), the function that draws every flashcard box in the program
// A flashcard: header label, centered lines, left-aligned lines, footer hint
void drawCard(const string& header, const string& color,
              const vector<string>& centered,
              const vector<string>& leftLines,
              const string& footer) {
    string bar = repeatStr("-", CARD_WIDTH + 2);
    cout << color << "+" << bar << "+" << RESET << "\n";
    if (!header.empty()) {   // empty header = a plain box with no title row
        cardRow(header, 'l', color);
        cout << color << "+" << bar << "+" << RESET << "\n";
    }
    cardBlank(color); 				//this is responsible for the card's color changing as well
    for (size_t i = 0; i < centered.size(); i++) cardRow(centered[i], 'c', color);
    cardBlank(color);
    for (size_t i = 0; i < leftLines.size(); i++) cardRow(leftLines[i], 'l', color);
    if (!leftLines.empty()) cardBlank(color);
    cout << color << "+" << bar << "+" << RESET << "\n";
    if (!footer.empty()) cout << "  " << footer << "\n";
}
//not really necessary but we added a progress bar for the students, it possibly might be really useful though
string progressBar(int current, int total) {
    int filled = (total == 0) ? 0 : (current * 20) / total;
    return "[" + repeatStr("#", filled) + repeatStr("-", 20 - filled) + "]";
}
//now this will give the users their remarks in strings, and sentences as we provided here
string getRemark(double percent) {
    if (percent <= 25.0) return "Needs a lot of review. Don't give up!";
    if (percent <= 50.0) return "Getting there. Keep practicing!";
    if (percent <= 75.0) return "Good job! Almost mastered it.";
    return "Excellent! You really know this topic!";
} //strings for remarks

// ---------------------------------------------------------------- accounts
string trim(const string& s) { //	these are helpers for the passwords
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

string toLowerStr(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

bool isBack(const string& s) { return s == BACK; }

// Reads one trimmed line (used for usernames and codes)
string readTrimmed(const string& prompt) {
    return trim(readLine(prompt));
}

// Reads a password. On Windows the characters show as * while typing. this is so that the users cant steal each other's information/password
string readPassword(const string& prompt) {
#if defined(_WIN32) && defined(HIDE_PASSWORD_INPUT)
    cout << prompt;
    string pw;
    while (true) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') break;               // ENTER
        if (ch == 0 || ch == 224) { _getch(); continue; }  // arrow / function keys
        if (ch == 8) {                                     // BACKSPACE
            if (!pw.empty()) {
                pw.erase(pw.size() - 1);
                cout << "\b \b";
            }
            continue;									//password helpers
        }
        if (ch >= 32 && ch < 127) {
            pw += (char)ch;
            cout << '*';
        }
    }
    cout << "\n";
    return pw;
#else
    return readTrimmed(prompt);
#endif
}

void screen(const string& title) {
    clearScreen();
    cout << "\n  " << BOLD << "=== " << title << " ===" << RESET << "\n\n";
}

void box(const string& text, const string& color = "") {
    cout << "\n  " << color << text << RESET << "\n";
} // 

// isasave ang lahat ng account sa users.txt para hindi mawala kapag na close ang program.
void saveUsers() {
    ofstream f(USERS_FILE.c_str());
    for (size_t i = 0; i < users.size(); i++)
        f << users[i].username << " " << users[i].password << " " << users[i].role << "\n";
}

// babasahin yung accounts galing sa users.txt everytime na bubuksan yung program.
// if theres no txt files our program will create default accounts at main(). :) Hi Sir
void loadUsers() {
    ifstream f(USERS_FILE.c_str());
    User u;
    while (f >> u.username >> u.password >> u.role) {
        if (u.role == "teacher" || u.role == "student")
            users.push_back(u);
    }
}

// ---------------------------------------------------------------- SAVING TESTS
// Ang tests.txt ang permanent storage ng mga test at questions.
// Hindi lang sa vector naka-save ang data, kaya babalik toh kahit i-close ang program.
// tests.txt layout (one line each, fields separated by |):
//   TEST|subject|topic
//   M|question|choiceA|choiceB|choiceC|choiceD|answerLetter|points     (multiple choice)
//   B|question|correctAnswer|points                                    (fill in the blank)
// Every M / B line belongs to the TEST line above it.

// the | character is the separator, so a typed | or \ is stored in a safe form
// I-convert ang special characters para hindi mapagkamalang separator ang | sa text.
string escapeField(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\') out += "\\\\";
        else if (s[i] == '|') out += "\\p";
        else out += s[i];
    }
    return out;
}

// Ibalik sa original na text ang mga character na ni-escape noong nag-save.
string unescapeField(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            i++;
            out += (s[i] == 'p') ? '|' : s[i];
        } else out += s[i];
    }
    return out;
}

// Hatiin ang isang line ng tests.txt gamit ang | bilang separator.
vector<string> splitFields(const string& line) {
    vector<string> out;
    string cur;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == '|') { out.push_back(cur); cur.clear(); }
        else cur += line[i];
    }
    out.push_back(cur);
    return out;
}

bool parseNumber(const string& s, int& out) {
    if (s.empty()) return false;
    char* end = 0;
    long v = strtol(s.c_str(), &end, 10);
    if (*end != '\0' || v < 0 || v > 100000) return false;
    out = (int)v;
    return true;
}

// Tawagin ito pagkatapos gumawa, mag-edit, o mag-delete ng test.
// Isinusulat nito ang lahat ng tests at questions sa tests.txt.
void saveTests() {
    ofstream f(TESTS_FILE.c_str());
    if (!f) {
        cout << "  [!] Warning: could not save to " << TESTS_FILE << ".\n";
        return;
    }
    for (size_t i = 0; i < tests.size(); i++) {
        const Test& t = tests[i];
        f << "TEST|" << escapeField(t.subject) << "|" << escapeField(t.topic) << "\n";
        for (size_t k = 0; k < t.questions.size(); k++) {
            const Question& q = t.questions[k];
            if (q.isBlank) {
                f << "B|" << escapeField(q.text) << "|" << escapeField(q.blankAnswer)
                  << "|" << q.points << "\n";
            } else {
                f << "M|" << escapeField(q.text);
                for (int c = 0; c < 4; c++) f << "|" << escapeField(q.choices[c]);
                f << "|" << q.answer << "|" << q.points << "\n";
            }
        }
    }
}

// Kapag wala pang tests.txt, normal lang iyon at magsisimula sa walang tests.
void loadTests() {
    ifstream f(TESTS_FILE.c_str());
    if (!f) return;
    string line;
    while (getline(f, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        if (line.empty()) continue;
        vector<string> p = splitFields(line);

        if (p[0] == "TEST" && p.size() == 3) {
            Test t;
            t.subject = unescapeField(p[1]);
            t.topic   = unescapeField(p[2]);
            tests.push_back(t);
        } else if (!tests.empty() && p[0] == "M" && p.size() == 8) {			//this is for saving and loading tests 
            Question q;															//this will also load the txt files stored from last session
            int pts;
            if (p[6].size() != 1 || p[6][0] < 'A' || p[6][0] > 'D' ||
                !parseNumber(p[7], pts)) continue;
            q.isBlank = false;
            q.text = unescapeField(p[1]);
            for (int c = 0; c < 4; c++) q.choices[c] = unescapeField(p[2 + c]);
            q.answer = p[6][0];
            q.points = pts;
            tests.back().questions.push_back(q);
        } else if (!tests.empty() && p[0] == "B" && p.size() == 4) {
            Question q;
            int pts;
            if (!parseNumber(p[3], pts)) continue;
            q.isBlank = true;
            q.text = unescapeField(p[1]);
            q.blankAnswer = unescapeField(p[2]);
            q.answer = 'A';
            q.points = pts;
            tests.back().questions.push_back(q);
        }
        // anything else is skipped
    }
}

void addDefaultUsers() { 		//now this is responsible for making the default accounts we can use when there arent any accounts made yet
    User t;
    t.username = "teacher"; t.password = "1234"; t.role = "teacher";
    users.push_back(t);
    User s;
    s.username = "student"; s.password = "1234"; s.role = "student";
    users.push_back(s);
}

// Usernames are not case-sensitive ("Orap" and "orap" are the same account)
int findUser(const string& name) {
    for (size_t i = 0; i < users.size(); i++)
        if (toLowerStr(users[i].username) == toLowerStr(name))
            return (int)i;
    return -1;
}

int teacherCount() {
    int n = 0;
    for (size_t i = 0; i < users.size(); i++)
        if (users[i].role == "teacher") n++;
    return n;
}

bool validUsername(const string& u, string& why) {
    if (u.size() < 3 || u.size() > 15) {
        why = "Username must be 3 to 15 characters long.";
        return false;
    }
    for (size_t i = 0; i < u.size(); i++) {
        if (!isalnum((unsigned char)u[i]) && u[i] != '_') {
            why = "Username can only use letters, numbers, and _ (no spaces).";
            return false;
        }
    }
    return true;
}

bool validPassword(const string& p, string& why) {
    if (p.size() < 4 || p.size() > 20) {
        why = "Password must be 4 to 20 characters long.";
        return false;
    }
    for (size_t i = 0; i < p.size(); i++) {
        if (isspace((unsigned char)p[i])) {
            why = "Password can't contain spaces.";
            return false;
        }
    }
    return true;
}

// Asks for a new password + confirmation.
// Returns false if the user typed /back at the first prompt.
bool askNewPassword(string& out, const string& notThis) {
    while (true) {
        string pw = readPassword("  New password (4-20 chars, no spaces): ");
        if (isBack(pw)) return false;

        string why;
        if (!validPassword(pw, why)) {
            cout << "  [!] " << why << "\n";
            continue;
        }
        if (!notThis.empty() && pw == notThis) {
            cout << "  [!] The new password must be different from the old one.\n";
            continue;
        }
        string again = readPassword("  Confirm password: ");
        if (isBack(again)) continue;
        if (again != pw) {
            cout << "  [!] Passwords do not match. Try again.\n";
            continue;
        }
        out = pw;
        return true;
    }
}

// Returns true if the login worked. 3 attempts, /back returns to the menu.
bool login(const string& role) {
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        screen(role == "teacher" ? "PROFESSOR LOGIN" : "STUDENT LOGIN");
        cout << "  (Type " << BACK << " to return to the main menu)\n\n";

        string username = readTrimmed("  Username: ");
        if (isBack(username)) return false;
        string password = readPassword("  Password: ");
        if (isBack(password)) return false;

        int idx = findUser(username);
        if (idx != -1 && users[idx].password == password) {
            if (users[idx].role == role) {
                currentUser = users[idx].username;
                return true;
            }
            // Correct password, wrong door: tell them which one to use
            cout << "\n  [i] That is a " << (users[idx].role == "teacher" ? "professor" : "student")
                 << " account. Please use \"Login as "
                 << (users[idx].role == "teacher" ? "Professor" : "Student")
                 << "\" instead.\n";
            pauseScreen();
            return false;
        }
        cout << "\n  " << RED << "[!] Wrong username or password. Attempts left: "
             << (MAX_ATTEMPTS - attempt) << RESET << "\n";
        pauseScreen();
    }
    box("[!] Too many failed attempts. Returning to the main menu.", RED);
    pauseScreen();
    return false;
}

void signUp(const string& role) {
    screen(role == "teacher" ? "PROFESSOR SIGN UP" : "STUDENT SIGN UP");
    cout << "  Type " << BACK << " at any prompt to go back one step.\n\n";

    string code, username, password;
    int firstStep  = (role == "teacher") ? 0 : 1;
    int step       = firstStep;      // 0 = access code, 1 = username, 2 = password
    int wrongCodes = 0;

    while (step <= 2) {
        if (step == 0) {
            code = readPassword("  Professor access code: ");
            if (isBack(code)) return;
            if (code != TEACHER_CODE) {
                wrongCodes++;
                if (wrongCodes >= MAX_ATTEMPTS) {
                    box("[!] Too many wrong codes. Returning to the main menu.", RED);
                    pauseScreen();
                    return;
                }
                cout << "  [!] Wrong access code.\n";
                continue;
            }
            step++;
        } else if (step == 1) {
            username = readTrimmed("  Username: ");
            if (isBack(username)) {
                if (step == firstStep) return;
                step--;
                continue;
            }
            string why;
            if (!validUsername(username, why)) {
                cout << "  [!] " << why << "\n";
                continue;
            }
            if (findUser(username) != -1) {
                cout << "  [!] That username is already taken.\n";
                continue;
            }
            step++;
        } else {
            if (!askNewPassword(password, "")) {
                step--;              // /back goes to the username
                continue;
            }
            step++;
        }
    }

    User u;
    u.username = username;
    u.password = password;
    u.role     = role;
    users.push_back(u);
    saveUsers();

    box("[OK] Account created! You can now log in.", GREEN);
    pauseScreen();
}

// ---- My Account (both roles)
void changePassword() {
    screen("CHANGE PASSWORD");
    cout << "  (Type " << BACK << " to go back)\n\n";

    int idx = findUser(currentUser);
    if (idx == -1) { box("[!] Account not found.", RED); return; }

    string oldPw = readPassword("  Current password: ");
    if (isBack(oldPw)) { box("[i] Cancelled. Password not changed."); return; }
    if (oldPw != users[idx].password) { box("[!] Wrong password.", RED); return; }

    string newPw;
    if (!askNewPassword(newPw, oldPw)) {
        box("[i] Cancelled. Password not changed.");
        return;
    }
    users[idx].password = newPw;
    saveUsers();
    box("[OK] Password changed.", GREEN);
}

// Returns true if the account was deleted (caller should log out)
bool deleteMyAccount() {
    screen("DELETE MY ACCOUNT");
    cout << "  (Type " << BACK << " to go back)\n\n";

    int idx = findUser(currentUser);
    if (idx == -1) { box("[!] Account not found.", RED); return false; }

    if (users[idx].role == "teacher" && teacherCount() <= 1) {
        box("[!] You are the only professor account, so it can't be deleted.", RED);
        return false;
    }

    string pw = readPassword("  Enter your password to continue: ");
    if (isBack(pw)) { box("[i] Cancelled."); return false; }
    if (pw != users[idx].password) { box("[!] Wrong password.", RED); return false; }

    if (!askYesNo("\n  Delete your account? This can't be undone.")) {
        box("[i] Cancelled. Your account was not deleted.");
        return false;
    }
    users.erase(users.begin() + idx);
    saveUsers();
    box("[OK] Your account was deleted.", GREEN);
    return true;
}

// Returns true if the account was deleted
bool myAccountMenu() {
    while (true) {
        screen("MY ACCOUNT - " + currentUser);
        cout << "  [1] Change password\n  [2] Delete my account\n  [0] Back\n\n";
        int choice = readInt("  Choice: ", 0, 2);

        if (choice == 0) return false;
        if (choice == 1) {
            changePassword();
            pauseScreen();
        } else {
            bool deleted = deleteMyAccount();
            pauseScreen();
            if (deleted) return true;
        }
    }
}

// ---- Professor: manage other people's accounts
void accountActions(int ui) {
    string name = users[ui].username;
    string role = users[ui].role;

    screen("ACCOUNT: " + name + " (" + (role == "teacher" ? "professor" : role) + ")");
    if (role == "student") cout << "  [1] Reset password\n";
    cout << "  [2] Delete account\n  [0] Back\n\n";
    int choice = readInt("  Choice: ", 0, 2);

    if (choice == 0) return;

    if (choice == 1) {
        if (role != "student") {
            cout << "  [!] You can only reset a student's password.\n";
            pauseScreen();
            return;
        }
        cout << "\n  Set a new password for " << name
             << " (type " << BACK << " to cancel).\n";
        string newPw;
        if (askNewPassword(newPw, "")) {
            users[ui].password = newPw;
            saveUsers();
            box("[OK] Password for " + name + " was reset.", GREEN);
        } else {
            box("[i] Cancelled. Password not changed.");
        }
        pauseScreen();
        return;
    }

    if (askYesNo("\n  Delete \"" + name + "\"? This can't be undone.")) {
        users.erase(users.begin() + ui);
        saveUsers();
        box("[OK] Account deleted.", GREEN);
    } else {
        box("[i] Cancelled. Nothing was deleted.");
    }
    pauseScreen();
}

void manageAccounts() {
    while (true) {
        screen("MANAGE ACCOUNTS");

        vector<int> indexes;          // everyone except me
        for (size_t i = 0; i < users.size(); i++) {
            if (toLowerStr(users[i].username) != toLowerStr(currentUser)) {
                indexes.push_back((int)i);
                cout << "  [" << indexes.size() << "] " << users[i].username
                     << " (" << (users[i].role == "teacher" ? "professor" : users[i].role) << ")\n";
            }
        }
        if (indexes.empty()) {
            cout << "  There are no other accounts.\n";
            pauseScreen();
            return;
        }
        cout << "  [0] Back\n\n";
        int pick = readInt("  Pick an account: ", 0, (int)indexes.size(), true);
        if (pick == 0) return;
        accountActions(indexes[pick - 1]);
    }
}

// ---------------------------------------------------------------- student side
void showFront(const Test& t, int idx) {
    const Question& q = t.questions[idx];
    int n = (int)t.questions.size();
    clearScreen();
    cout << "\n  " << BOLD << t.subject << RESET << "   "
         << progressBar(idx, n) << "  Card " << (idx + 1) << "/" << n << "\n"
         << "  Topic: " << t.topic << "\n\n";

    vector<string> centered = wrapText(q.text, CARD_WIDTH);
    vector<string> choices;
    for (int c = 0; c < 4 && !q.isBlank; c++) {
        vector<string> w = wrapText(q.choices[c], CARD_WIDTH - 3);
        for (size_t k = 0; k < w.size(); k++)
            choices.push_back((k == 0 ? string(1, 'A' + c) + ". " : "   ") + w[k]);
    }
    drawCard(string(q.isBlank ? "FILL IN THE BLANK" : "QUESTION") + "  (" + toStr(q.points) + " pt" + (q.points == 1 ? "" : "s") + ")",
             CYAN, centered, choices, "");
}
// Cleans a typed answer so it can be compared and then removes spaces at the
// start/end and ignores capital letters ("  Four " and "four" match) this makes it flexible so that it wont be a hassle because of case sensitivity
string cleanAnswer(const string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    string out;
    for (size_t i = start; i <= end; i++)
        out += (char)tolower(s[i]);
    return out;
}

// Turns a question and its correct answer into one sentence.
// "The derivative of 2x + 5 is ____."  + "2"  ->  "The derivative of 2x + 5 is 2." This is just an example so we dont get confused-abi
// "What is the derivative of 2x + 5?" + "2"  ->  "The derivative of 2x + 5 is 2."
// Other questions become "Question? Answer." Basically this will add the answer to the question without the ? so it become
//a full on flashcard result, thats the idea we tried here.
string makeAnswerSentence(const string& question, const string& answer) {
    size_t blank = question.find('_');                 // is there a ____ blank?
    if (blank != string::npos) {
        size_t end = blank;
        while (end < question.size() && question[end] == '_') end++;
        return question.substr(0, blank) + answer + question.substr(end);
    }

    string q = question;
    while (!q.empty() && (q[q.size() - 1] == ' ' || q[q.size() - 1] == '?'))
        q.erase(q.size() - 1);                              // remove the "?" at the end

    string lower = q;
    for (size_t i = 0; i < lower.size(); i++)
        lower[i] = tolower(lower[i]);

    string rest;
    string verb;
    if (lower.compare(0, 8, "what is ") == 0) {
        rest = q.substr(8);
        verb = " is ";
    } else if (lower.compare(0, 9, "what are ") == 0) {
        rest = q.substr(9);
        verb = " are ";
    } else {
        return q + "? " + answer + ".";           // fallback for other questions
    }

    if (!rest.empty()) rest[0] = toupper(rest[0]);     // capital first letter
    return rest + verb + answer + ".";
}

//this is the true or false functions this would determine whether the user's answers are correct or incorrect
void showBack(const Question& q, const string& given, bool correct) {
    string answerText = q.isBlank ? q.blankAnswer : q.choices[q.answer - 'A'];

    // the box will only hold the answer sentence, the results will go under after answering
    vector<string> sentence = wrapText(makeAnswerSentence(q.text, answerText), CARD_WIDTH);
    vector<string> none;
    drawCard("", correct ? GREEN : RED, sentence, none, "");

    // this line of code I put makes the results go BELOW the box after answering.
    cout << "\n  " << (correct ? GREEN : RED) << (correct ? "CORRECT!" : "WRONG") << RESET << "\n";
    string shownAnswer = given;
    if (!q.isBlank && !given.empty())   // multiple choice: show the letter and what it says
        shownAnswer = given + " (" + q.choices[toupper(given[0]) - 'A'] + ")";
    cout << "  Your answer:    " << (shownAnswer.empty() ? "(no answer)" : shownAnswer) << "\n";
    if (q.isBlank) cout << "  Correct answer: " << answerText << "\n";
    else cout << "  Correct answer: " << q.answer << ". " << answerText << "\n";
    cout << "  Points earned:  " << (correct ? "+" + toStr(q.points) : "0") << "\n";
}
//Now for this part, this is the test, whereas this is where it shows the answers whether they are true or false.
void takeTest(const Test& t) {
    int total = 0, maxPts = 0;
    int n = (int)t.questions.size();

    for (int i = 0; i < n; i++) {
        const Question& q = t.questions[i];
        maxPts += q.points;
        showFront(t, i);
        string given;
        bool correct = false;
        cout << "\n  (type " << BACK << " to leave the test)\n";
        while (true) {
            string input = readLine(q.isBlank ? "  Your answer: " : "  Your answer (A-D): ");
            if (trim(input) == BACK) {                    // wrong test chosen? they can leave
                if (askYesNo("  Leave this test? Your progress will be lost.")) return;
                continue;
            }
            if (q.isBlank) {
                given = input;
                correct = (cleanAnswer(given) == cleanAnswer(q.blankAnswer));
                break;
            }
            string v = trim(input);
            if (v.size() == 1 && toupper(v[0]) >= 'A' && toupper(v[0]) <= 'D') {
                char letter = (char)toupper(v[0]);
                given = string(1, letter);
                correct = (letter == q.answer);
                break;
            }
            cout << "Please enter A, B, C, or D.\n";
        }
        if (correct) total += q.points;

        // "flip" the card
        clearScreen();//this will clear the screen, giving that clear program look -Abraham
        cout << "\n  " << BOLD << t.subject << RESET << "   "
             << progressBar(i + 1, n) << "  Card " << (i + 1) << "/" << n << "\n"
             << "  Topic: " << t.topic << "\n\n";
        showBack(q, given, correct);
        pauseScreen();
    }
//now this will be responsible for the scoring, remarks and more.
    double percent = (maxPts == 0) ? 0 : (100.0 * total / maxPts);
    clearScreen();
    cout << "\n";
    vector<string> centered;
    centered.push_back(t.subject);
    centered.push_back("Topic: " + t.topic);
    centered.push_back("");
    centered.push_back("Score: " + toStr(total) + " / " + toStr(maxPts));
    centered.push_back(toStr((int)percent) + "%");
    vector<string> remark = wrapText(getRemark(percent), CARD_WIDTH);
    drawCard("RESULTS", YELLOW, centered, remark, "");
    pauseScreen();
}
//this is the student's menu they will see everything that students can do
void chooseTestMenu() {
    while (true) {
        clearScreen();
        cout << "\n  Available tests:\n\n";
        if (tests.empty()) {
            cout << "  No tests available yet. Ask your teacher to make one.\n";
            pauseScreen();
            return;
        }
        for (size_t i = 0; i < tests.size(); i++)
            cout << "  [" << i + 1 << "] " << tests[i].subject
                 << " - Topic: " << tests[i].topic
                 << " (" << tests[i].questions.size() << " cards)\n";
        cout << "  [0] Back\n\n";
        int c = readInt("  Choose a test: ", 0, (int)tests.size(), true);
        if (c == 0) return;
        if (tests[c - 1].questions.empty()) {
            cout << "  That test has no questions yet.\n";
            pauseScreen();
        } else {
            takeTest(tests[c - 1]);
        }
    }
}

void studentMenu() {
    while (true) {
        clearScreen();
        cout << "\n  Welcome, Student " << BOLD << currentUser << RESET << "!\n\n"
             << "  [1] Take a test\n  [2] My Account\n  [0] Back (logout)\n\n";
        int c = readInt("  Choice: ", 0, 2);
        if (c == 0) break;
        if (c == 1) chooseTestMenu();
        else if (myAccountMenu()) break;     // account deleted -> logout
    }
    currentUser = "";
}

//  professor option menu
// ---- "go back" helpers: typing /back at these prompts returns false
bool readTextBack(const string& prompt, string& out) {
    while (true) {
        string v = trim(readLine(prompt));
        if (v == BACK) return false;
        if (v.empty()) { cout << "  This can't be empty.\n"; continue; }
        out = v;
        return true;
    }
}

bool readIntBack(const string& prompt, int lo, int hi, int& out) {
    while (true) {
        string v = trim(readLine(prompt));
        if (v == BACK) return false;
        int n;
        if (parseInt(v, n) && n >= lo && n <= hi) { out = n; return true; }
        cout << "  Please enter a number from " << lo << " to " << hi << " (or " << BACK << ").\n";
    }
}

bool readLetterBack(const string& prompt, char& out) {
    while (true) {
        string v = trim(readLine(prompt));
        if (v == BACK) return false;
        if (v.size() == 1 && toupper(v[0]) >= 'A' && toupper(v[0]) <= 'D') {
            out = (char)toupper(v[0]);
            return true;
        }
        cout << "  Please enter A, B, C, or D (or " << BACK << ").\n";
    }
}

// Walks the professor through ONE question, one step at a time.
// Typing /back (or 0 at the question type) goes back one step.
// Returns false if they backed out of the first step; nothing is changed in that case.
bool inputQuestion(Question& result) {
    Question q = result;          // work on a copy, only keep it when every step is done
    // steps: 0 type | 1 question | 2 blank answer, or choices A-D (2-5) then 6 correct letter | 7 points
    int step = 0;
    cout << "  (type " << BACK << " at any step to go back)\n";

    while (step <= 7) {
        if (step == 0) {
            cout << "  [1] Multiple choice\n  [2] Fill in the blank\n  [0] Go back\n";
            int t = readInt("  Question type: ", 0, 2, true);
            if (t == 0) return false;
            q.isBlank = (t == 2);
            step = 1;
        } else if (step == 1) {
            if (q.isBlank) cout << "  Instruction: type ____ where the blank goes .\n";
            if (!readTextBack("  Question: ", q.text)) { step = 0; continue; }
            step = 2;
        } else if (step == 2 && q.isBlank) {
            if (!readTextBack("  Correct answer: ", q.blankAnswer)) { step = 1; continue; }
            step = 7;
        } else if (step <= 5) {                       // choices A to D
            int c = step - 2;
            if (!readTextBack("  Choice " + string(1, 'A' + c) + ": ", q.choices[c])) { step--; continue; }
            step++;
        } else if (step == 6) {
            if (!readLetterBack("  Correct answer (A-D): ", q.answer)) { step = 5; continue; }
            step = 7;
        } else {                                      // step 7: points
            if (!readIntBack("  Points (1-100): ", 1, 100, q.points)) { step = q.isBlank ? 2 : 6; continue; }
            step = 8;
        }
    }
    result = q;
    return true;
}
// Shows the 4 subjects and returns the one picked ("" = cancelled)
string pickSubject(const string& prompt) {
    cout << prompt << "\n\n";
    for (int i = 0; i < NUM_SUBJECTS; i++)
        cout << "  [" << i + 1 << "] " << SUBJECTS[i] << "\n";
    cout << "  [0] Cancel\n\n";
    int c = readInt("  Subject: ", 0, NUM_SUBJECTS, true);
    return (c == 0) ? "" : SUBJECTS[c - 1];
}

// An example topic that fits each subject (shown when asking for the topic)
string topicExample(const string& subject) {
    if (subject == "Science")     return "Chemistry";
    if (subject == "Mathematics") return "Algebra";
    if (subject == "Computer")    return "Programming";
    if (subject == "English")     return "Grammar";
    return "Chemistry";
}

// Asks for the topic until something is typed
bool askTopic(const string& prompt, string& out) {
    while (true) {
        string topic = readTrimmed(prompt);
        if (topic == BACK) return false;
        if (!topic.empty()) { out = topic; return true; }
        cout << "  Topic can't be empty.\n";
    }
}

//this will be the test creating section
// Steps: subject -> topic -> cards.  /back (or [0]) goes back one step.
// Professor feature: gumawa ng test, idagdag ang questions, at i-save sa TXT file.
void createTest() {
    Test t;
    int step = 0;                     // 0 = subject, 1 = topic, 2 = cards
    while (true) {
        if (step == 0) {
            clearScreen();
            cout << "\n  === CREATE TEST ===\n\n";
            t.subject = pickSubject("  Choose a subject:");
            if (t.subject.empty()) return;            // cancelled, nothing saved
            step = 1;
        } else if (step == 1) {
            clearScreen();
            cout << "\n  === CREATE TEST ===\n\n  Subject: " << t.subject
                 << "\n  (type " << BACK << " to choose the subject again)\n\n";
            if (!askTopic("  Topic for " + t.subject + " (e.g. " + topicExample(t.subject) + "): ", t.topic)) {
                step = 0;
                continue;
            }
            step = 2;
        } else {
            cout << "\n  -- Card " << t.questions.size() + 1 << " --\n";
            Question q;
            if (!inputQuestion(q)) {
                if (t.questions.empty()) { step = 1; continue; }   // back to the topic
                cout << "\n  Stopped adding cards.\n";            // backed out of an extra card
            } else {
                t.questions.push_back(q);
                if (askYesNo("\n  Add another question?")) continue;
            }
            break;
        }
    }
    tests.push_back(t);
    saveTests();
    cout << "\n  Test saved!\n";
    pauseScreen();
}
//this is where the professor will pick tests 
int pickTest(const string& action) {
    if (tests.empty()) {
        cout << "\n  No tests yet.\n";
        pauseScreen();
        return -1;
    }
    cout << "\n";
    for (size_t i = 0; i < tests.size(); i++)
        cout << "  [" << i + 1 << "] " << tests[i].subject
             << " - Topic: " << tests[i].topic << "\n";
    cout << "  [0] Cancel\n\n";
    int c = readInt("  Which test to " + action + "? ", 0, (int)tests.size(), true);
    return c - 1;   // -1 means cancel
}
//test editor for the professor
// Professor feature: baguhin ang test o questions; bawat pagbabago ay sini-save.
void editTest() {
    clearScreen();
    cout << "\n  === EDIT TEST ===\n";
    int idx = pickTest("edit");
    if (idx < 0) return;
    Test& t = tests[idx];

    while (true) { 							//also forgot to mention that it will check whether theres an existing test or not.
        clearScreen();
        cout << "\n  Editing: " << t.subject << " - Topic: " << t.topic << "\n\n";
        for (size_t i = 0; i < t.questions.size(); i++)
            cout << "  " << i + 1 << ". " << t.questions[i].text
                 << " (" << t.questions[i].points << " pts)\n";
        cout << "\n  [1] Change subject\n  [2] Change topic\n  [3] Add question\n"
                "  [4] Edit a question\n  [5] Delete a question\n  [0] Back\n\n";
        int c = readInt("  Choice: ", 0, 5, true);
        if (c == 0) return;

        if (c == 1) {
            cout << "\n";
            string sub = pickSubject("  Choose the new subject:");
            if (!sub.empty()) t.subject = sub;
        } else if (c == 2) {
            string newTopic;
            if (askTopic("  New topic for " + t.subject + " (e.g. " + topicExample(t.subject) + ", or " + BACK + " to cancel): ", newTopic))
                t.topic = newTopic;
        } else if (c == 3) {
            Question q;
            if (inputQuestion(q)) t.questions.push_back(q);
        } else if (t.questions.empty()) {
            cout << "  No questions to change.\n";
            pauseScreen();
        } else {
            int n = readInt("  Question number (0 = go back): ", 0, (int)t.questions.size(), true);
            if (n == 0) {
                // went back, nothing changes
            } else if (c == 4) {
                inputQuestion(t.questions[n - 1]);      // only changed if every step is finished
            } else if (askYesNo("  Delete question " + toStr(n) + "?")) {
                t.questions.erase(t.questions.begin() + (n - 1));
            }
        }
        saveTests();                      // keep tests.txt up to date after every change
    }
}
//test deleting section
// Professor feature: burahin ang test at i-update agad ang tests.txt.
void deleteTest() {
    clearScreen();
    cout << "\n  === DELETE TEST ===\n";
    int idx = pickTest("delete");
    if (idx < 0) return;
    if (askYesNo("  Really delete \"" + tests[idx].subject + "\"?")) {
        tests.erase(tests.begin() + idx);
        saveTests();
        cout << "  Deleted.\n";
        pauseScreen();
    }
}
// shows one test with every question and its correct answer
void showTestDetails(const Test& t) {
    clearScreen();
    cout << "\n  === " << t.subject << " ===\n  Topic: " << t.topic
         << "\n  Cards: " << t.questions.size() << "\n";
    if (t.questions.empty()) cout << "\n  (no questions yet)\n";
    for (size_t i = 0; i < t.questions.size(); i++) {
        const Question& q = t.questions[i];
        cout << "\n  " << BOLD << "Q" << i + 1 << ". " << q.text << RESET
             << "  (" << q.points << " pt" << (q.points == 1 ? "" : "s") << ")\n";
        if (q.isBlank) {
            cout << "     Type: Fill in the blank\n"
                 << "     " << GREEN << "Answer: " << q.blankAnswer << RESET << "\n";
        } else {
            for (int c = 0; c < 4; c++) {
                bool right = (q.answer == 'A' + c);
                cout << "     " << (right ? GREEN : "") << (char)('A' + c) << ". " << q.choices[c]
                     << (right ? "   <-- correct" : "") << RESET << "\n";
            }
        }
    }
}

// view tests menu: pick a test to see its questions and answers
void viewTests() {
    while (true) {
        clearScreen();
        cout << "\n  === VIEW TESTS ===\n";
        int idx = pickTest("view");
        if (idx < 0) return;
        showTestDetails(tests[idx]);
        pauseScreen();
    }
}

//professor's menu
void teacherMenu() {
    while (true) {
        clearScreen();
        cout << "\n  Welcome, Professor " << BOLD << currentUser << RESET << "!\n\n"
             << "  [1] Create test\n  [2] Edit test\n"
             << "  [3] Delete test\n  [4] View tests\n"
             << "  [5] Manage Accounts\n  [6] My Account\n  [0] Back (logout)\n\n";
        int c = readInt("  Choice: ", 0, 6);
        switch (c) {
            case 1: createTest(); break;
            case 2: editTest(); break;
            case 3: deleteTest(); break;
            case 4: viewTests(); break;
            case 5: manageAccounts(); break;
            case 6:
                if (myAccountMenu()) { currentUser = ""; return; }   // account deleted -> logout
                break;
            case 0: currentUser = ""; return;
        }
    }
}

// main program 
// Main function: i-set up ang console at i-load ang saved users/tests bago ang menu.
int main() {
    setupConsole();

    loadUsers();                      // Kunin ang saved accounts mula sa users.txt.
    loadTests();                      // Kunin ang saved tests mula sa tests.txt.
    if (users.empty()) {              // first run: no users.txt yet
        addDefaultUsers();
        saveUsers();
    }

    while (true) {
        clearScreen();
        vector<string> centered;
        centered.push_back("FLASHCARD PROGRAM GROUP 4");
        vector<string> menu;
        menu.push_back("[1] Login as Professor/Facilitator");
        menu.push_back("[2] Login as Student");
        menu.push_back("[3] Sign up as Professor/Facilitator");
        menu.push_back("[4] Sign up as Student");
        menu.push_back("[0] Exit");
        cout << "\n";
        drawCard("MAIN MENU", CYAN, centered, menu, "");
        cout << "  Menus: choose 0 to go back.   Typing prompts: type " << BACK << ".\n";
        int choice = readInt("\n  Enter choice: ", 0, 4);

        if (choice == 0) {
            cout << "\n  The user has logged out, program terminated.\n";
            break;
        }
        switch (choice) {
            case 1: if (login("teacher")) teacherMenu(); break;
            case 2: if (login("student")) studentMenu(); break;
            case 3: signUp("teacher"); break;
            case 4: signUp("student"); break;
        }
    }
    return 0;
}
