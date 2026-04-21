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
    int rank = 0;

    int solvedCount = 0;
    long long penalty = 0;
    vector<int> solveTimes;

    struct ProblemInfo {
        bool solved = false;
        int failedAttempts = 0;
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
    for (size_t i = 0; i < a->solveTimes.size() && i < b->solveTimes.size(); ++i) {
        if (a->solveTimes[i] != b->solveTimes[i])
            return a->solveTimes[i] < b->solveTimes[i];
    }
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
    vector<Team*> scoreboard;

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
                for (int i = 0; i < (int)scoreboard.size(); ++i) {
                    scoreboard[i]->rank = i + 1;
                }
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            string probName, by, teamName, with, statusStr, at;
            int time;
            cin >> probName >> by >> teamName >> with >> statusStr >> at >> time;
            int pId = probName[0] - 'A';
            auto it = teamNameToId.find(teamName);
            if (it == teamNameToId.end()) continue;
            Team* t = teams[it->second];
            Status s = stringToStatus(statusStr);

            t->allSubmissions.push_back({pId, s, time});

            auto& p = t->problems[pId];
            if (!p.solved) {
                if (frozen) {
                    if (!p.frozen) {
                        p.frozen = true;
                        p.failedBeforeFreeze = p.failedAttempts;
                        p.submissionsAfterFreeze = 0;
                        p.pendingSubmissions.clear();
                    }
                    p.pendingSubmissions.push_back({pId, s, time});
                    p.submissionsAfterFreeze++;
                } else {
                    if (s == Accepted) {
                        p.solved = true;
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
            for (int i = 0; i < (int)scoreboard.size(); ++i) scoreboard[i]->rank = i + 1;
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
                for (int i = 0; i < (int)scoreboard.size(); ++i) scoreboard[i]->rank = i + 1;

                auto printScoreboard = [&]() {
                    for (int i = 0; i < (int)scoreboard.size(); ++i) {
                        Team* t = scoreboard[i];
                        cout << t->name << " " << i + 1 << " " << t->solvedCount << " " << t->penalty;
                        for (int j = 0; j < problemCount; ++j) {
                            auto& pr = t->problems[j];
                            cout << " ";
                            if (pr.frozen) {
                                if (pr.failedBeforeFreeze == 0) cout << "0/" << pr.submissionsAfterFreeze;
                                else cout << "-" << pr.failedBeforeFreeze << "/" << pr.submissionsAfterFreeze;
                            } else if (pr.solved) {
                                if (pr.failedAttempts == 0) cout << "+";
                                else cout << "+" << pr.failedAttempts;
                            } else {
                                if (pr.failedAttempts == 0) cout << ".";
                                else cout << "-" << pr.failedAttempts;
                            }
                        }
                        cout << "\n";
                    }
                };

                printScoreboard();

                int lastIdx = (int)scoreboard.size() - 1;
                while (true) {
                    while (lastIdx >= 0) {
                        bool hasFrozen = false;
                        for (int j = 0; j < problemCount; ++j) {
                            if (scoreboard[lastIdx]->problems[j].frozen) {
                                hasFrozen = true;
                                break;
                            }
                        }
                        if (hasFrozen) break;
                        lastIdx--;
                    }
                    if (lastIdx < 0) break;

                    Team* targetTeam = scoreboard[lastIdx];
                    int targetPos = lastIdx;

                    int targetProb = -1;
                    for (int j = 0; j < problemCount; ++j) {
                        if (targetTeam->problems[j].frozen) {
                            targetProb = j;
                            break;
                        }
                    }

                    auto& pr = targetTeam->problems[targetProb];
                    pr.frozen = false;
                    for (auto& sub : pr.pendingSubmissions) {
                        if (sub.status == Accepted) {
                            pr.solved = true;
                            targetTeam->solvedCount++;
                            targetTeam->penalty += (long long)pr.failedAttempts * 20 + sub.time;
                            targetTeam->solveTimes.push_back(sub.time);
                            sort(targetTeam->solveTimes.begin(), targetTeam->solveTimes.end(), greater<int>());
                            break;
                        } else {
                            pr.failedAttempts++;
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
                for (int i = 0; i < (int)scoreboard.size(); ++i) scoreboard[i]->rank = i + 1;
                printScoreboard();
                frozen = false;
            }
        } else if (cmd == "QUERY_RANKING") {
            string teamName;
            cin >> teamName;
            auto it = teamNameToId.find(teamName);
            if (it == teamNameToId.end()) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                }
                cout << teamName << " NOW AT RANKING " << teams[it->second]->rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, probPart, andPart, statusPart;
            cin >> teamName >> where >> probPart >> andPart >> statusPart;
            auto it = teamNameToId.find(teamName);
            if (it == teamNameToId.end()) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                string probName = probPart.substr(8);
                string statusStr = statusPart.substr(7);
                Team* t = teams[it->second];
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
