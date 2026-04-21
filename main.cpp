#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

enum Status {
    Accepted,
    Wrong_Answer,
    Runtime_Error,
    Time_Limit_Exceed,
    Other
};

Status stringToStatus(const string& s) {
    if (s == "Accepted") return Accepted;
    if (s == "Wrong_Answer") return Wrong_Answer;
    if (s == "Runtime_Error") return Runtime_Error;
    if (s == "Time_Limit_Exceed") return Time_Limit_Exceed;
    return Other;
}

string statusToString(Status s) {
    if (s == Accepted) return "Accepted";
    if (s == Wrong_Answer) return "Wrong_Answer";
    if (s == Runtime_Error) return "Runtime_Error";
    if (s == Time_Limit_Exceed) return "Time_Limit_Exceed";
    return "";
}

struct Submission {
    int problemId;
    Status status;
    int time;
};

struct Team {
    string name;
    int id;

    // Real-time stats (visible)
    int solvedCount = 0;
    long long penalty = 0;
    vector<int> solveTimes; // Sorted descending

    // Problem stats
    struct ProblemInfo {
        bool solved = false;
        int firstSolvedTime = -1;
        int failedAttempts = 0;

        // For frozen logic
        bool frozen = false;
        int failedBeforeFreeze = 0;
        int submissionsAfterFreeze = 0;
        vector<Submission> pendingSubmissions;
    };

    vector<ProblemInfo> problems;
    vector<Submission> allSubmissions;

    Team(string n, int i) : name(n), id(i) {}
};

bool compareTeams(const Team* a, const Team* b) {
    if (a->solvedCount != b->solvedCount)
        return a->solvedCount > b->solvedCount;
    if (a->penalty != b->penalty)
        return a->penalty < b->penalty;
    // Tie-break: compare solve times in descending order
    for (size_t i = 0; i < a->solveTimes.size() && i < b->solveTimes.size(); ++i) {
        if (a->solveTimes[i] != b->solveTimes[i])
            return a->solveTimes[i] < b->solveTimes[i];
    }
    // Lexicographical order
    return a->name < b->name;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int duration = 0, problemCount = 0;
    bool started = false;
    bool frozen = false;

    vector<Team*> teams;
    map<string, int> teamNameToId;
    vector<Team*> scoreboard; // Current rankings

    string cmd;
    while (cin >> cmd) {
        if (cmd == "ADDTEAM") {
            string teamName;
            cin >> teamName;
            if (started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (teamNameToId.count(teamName)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                int id = teams.size();
                teamNameToId[teamName] = id;
                teams.push_back(new Team(teamName, id));
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            string dummy;
            cin >> dummy >> duration >> dummy >> problemCount;
            if (started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                started = true;
                for (auto t : teams) {
                    t->problems.assign(problemCount, Team::ProblemInfo());
                }
                scoreboard = teams;
                sort(scoreboard.begin(), scoreboard.end(), [](Team* a, Team* b) {
                    return a->name < b->name;
                });
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            string probName, by, teamName, with, statusStr, at;
            int time;
            cin >> probName >> by >> teamName >> with >> statusStr >> at >> time;
            int pId = probName[0] - 'A';
            if (teamNameToId.find(teamName) == teamNameToId.end()) continue;
            int tId = teamNameToId[teamName];
            Team* t = teams[tId];
            Status s = stringToStatus(statusStr);

            Submission sub = {pId, s, time};
            t->allSubmissions.push_back(sub);

            auto& p = t->problems[pId];
            if (!p.solved) {
                if (frozen) {
                    if (!p.frozen) {
                        p.frozen = true;
                        p.failedBeforeFreeze = p.failedAttempts;
                        p.submissionsAfterFreeze = 0;
                        p.pendingSubmissions.clear();
                    }
                    p.pendingSubmissions.push_back(sub);
                    p.submissionsAfterFreeze++;
                } else {
                    if (s == Accepted) {
                        p.solved = true;
                        p.firstSolvedTime = time;
                        t->solvedCount++;
                        t->penalty += (long long)p.failedAttempts * 20 + time;
                        t->solveTimes.push_back(time);
                        sort(t->solveTimes.begin(), t->solveTimes.end(), greater<int>());
                    } else {
                        p.failedAttempts++;
                    }
                }
            }
        } else if (cmd == "FLUSH") {
            cout << "[Info]Flush scoreboard.\n";
            sort(scoreboard.begin(), scoreboard.end(), compareTeams);
        } else if (cmd == "FREEZE") {
            if (frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                frozen = true;
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                cout << "[Info]Scroll scoreboard.\n";
                sort(scoreboard.begin(), scoreboard.end(), compareTeams);

                auto printScoreboard = [&]() {
                    for (int i = 0; i < (int)scoreboard.size(); ++i) {
                        Team* t = scoreboard[i];
                        cout << t->name << " " << i + 1 << " " << t->solvedCount << " " << t->penalty;
                        for (int j = 0; j < problemCount; ++j) {
                            auto& p = t->problems[j];
                            cout << " ";
                            if (p.frozen) {
                                cout << "-" << p.failedBeforeFreeze << "/" << p.submissionsAfterFreeze;
                            } else if (p.solved) {
                                if (p.failedAttempts == 0) cout << "+";
                                else cout << "+" << p.failedAttempts;
                            } else {
                                if (p.failedAttempts == 0) cout << ".";
                                else cout << "-" << p.failedAttempts;
                            }
                        }
                        cout << "\n";
                    }
                };

                printScoreboard();

                while (true) {
                    Team* targetTeam = nullptr;
                    int targetPos = -1;

                    for (int i = (int)scoreboard.size() - 1; i >= 0; --i) {
                        bool hasFrozen = false;
                        for (int j = 0; j < problemCount; ++j) {
                            if (scoreboard[i]->problems[j].frozen) {
                                hasFrozen = true;
                                break;
                            }
                        }
                        if (hasFrozen) {
                            targetTeam = scoreboard[i];
                            targetPos = i;
                            break;
                        }
                    }

                    if (!targetTeam) break;

                    int targetProb = -1;
                    for (int j = 0; j < problemCount; ++j) {
                        if (targetTeam->problems[j].frozen) {
                            targetProb = j;
                            break;
                        }
                    }

                    auto& p = targetTeam->problems[targetProb];
                    p.frozen = false;
                    bool becameSolved = false;
                    for (auto& sub : p.pendingSubmissions) {
                        if (sub.status == Accepted) {
                            p.solved = true;
                            p.firstSolvedTime = sub.time;
                            targetTeam->solvedCount++;
                            targetTeam->penalty += (long long)p.failedAttempts * 20 + sub.time;
                            targetTeam->solveTimes.push_back(sub.time);
                            sort(targetTeam->solveTimes.begin(), targetTeam->solveTimes.end(), greater<int>());
                            becameSolved = true;
                            break;
                        } else {
                            p.failedAttempts++;
                        }
                    }

                    int currentPos = targetPos;
                    while (currentPos > 0 && compareTeams(targetTeam, scoreboard[currentPos - 1])) {
                        swap(scoreboard[currentPos], scoreboard[currentPos - 1]);
                        currentPos--;
                    }

                    if (currentPos != targetPos) {
                        cout << targetTeam->name << " " << scoreboard[currentPos + 1]->name << " "
                             << targetTeam->solvedCount << " " << targetTeam->penalty << "\n";
                    }
                }

                printScoreboard();
                frozen = false;
            }
        } else if (cmd == "QUERY_RANKING") {
            string teamName;
            cin >> teamName;
            if (teamNameToId.find(teamName) == teamNameToId.end()) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                }
                int rank = -1;
                for (int i = 0; i < (int)scoreboard.size(); ++i) {
                    if (scoreboard[i]->name == teamName) {
                        rank = i + 1;
                        break;
                    }
                }
                cout << teamName << " NOW AT RANKING " << rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, probPart, andPart, statusPart;
            cin >> teamName >> where >> probPart >> andPart >> statusPart;
            if (teamNameToId.find(teamName) == teamNameToId.end()) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                string probName = probPart.substr(8);
                string statusStr = statusPart.substr(7);

                Team* t = teams[teamNameToId[teamName]];
                Submission* lastMatch = nullptr;
                for (int i = (int)t->allSubmissions.size() - 1; i >= 0; --i) {
                    auto& sub = t->allSubmissions[i];
                    bool probMatch = (probName == "ALL" || (sub.problemId == probName[0] - 'A'));
                    bool statusMatch = (statusStr == "ALL" || (sub.status == stringToStatus(statusStr)));
                    if (probMatch && statusMatch) {
                        lastMatch = &sub;
                        break;
                    }
                }

                if (lastMatch) {
                    cout << teamName << " " << (char)(lastMatch->problemId + 'A') << " "
                         << statusToString(lastMatch->status) << " " << lastMatch->time << "\n";
                } else {
                    cout << "Cannot find any submission.\n";
                }
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }

    for (auto t : teams) delete t;
    return 0;
}
