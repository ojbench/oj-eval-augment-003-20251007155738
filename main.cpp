#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
};

struct ProblemStatus {
    bool solved = false;
    int wrongAttempts = 0;
    int solveTime = 0;
    int frozenSubmissions = 0;
    int wrongBeforeFreeze = 0;
    vector<Submission> allSubmissions;
    vector<Submission> frozenSubs;  // Submissions made during freeze
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    int solvedCount = 0;
    int penaltyTime = 0;
    vector<int> solveTimes;
    int ranking = 0;
    vector<Submission> allSubmissions;  // All submissions in order
    int frozenProblemCount = 0;  // Count of problems with frozen submissions
};

class ICPCSystem {
private:
    unordered_map<string, Team> teams;
    vector<string> teamOrder;
    bool started = false;
    bool frozen = false;
    int durationTime = 0;
    int problemCount = 0;
    vector<string> problemNames;
    int freezeTime = -1;
    unordered_set<string> teamsWithFrozenProblems;  // Track teams with frozen problems
    
    void calculateTeamStats(Team& team, bool includeFrozen) {
        team.solvedCount = 0;
        team.penaltyTime = 0;
        team.solveTimes.clear();
        
        for (auto& p : team.problems) {
            if (p.second.solved) {
                team.solvedCount++;
                team.penaltyTime += p.second.solveTime + 20 * p.second.wrongAttempts;
                team.solveTimes.push_back(p.second.solveTime);
            }
        }
        
        sort(team.solveTimes.rbegin(), team.solveTimes.rend());
    }
    
    bool compareTeams(const Team& a, const Team& b) {
        if (a.solvedCount != b.solvedCount) return a.solvedCount > b.solvedCount;
        if (a.penaltyTime != b.penaltyTime) return a.penaltyTime < b.penaltyTime;
        
        for (size_t i = 0; i < min(a.solveTimes.size(), b.solveTimes.size()); i++) {
            if (a.solveTimes[i] != b.solveTimes[i]) {
                return a.solveTimes[i] < b.solveTimes[i];
            }
        }
        
        return a.name < b.name;
    }
    
    void updateRankings() {
        vector<Team*> sortedTeams;
        for (auto& name : teamOrder) {
            sortedTeams.push_back(&teams[name]);
        }
        
        sort(sortedTeams.begin(), sortedTeams.end(), [this](Team* a, Team* b) {
            return compareTeams(*a, *b);
        });
        
        for (size_t i = 0; i < sortedTeams.size(); i++) {
            sortedTeams[i]->ranking = i + 1;
        }
    }
    
    void printScoreboard() {
        vector<Team*> sortedTeams;
        for (auto& name : teamOrder) {
            sortedTeams.push_back(&teams[name]);
        }
        
        sort(sortedTeams.begin(), sortedTeams.end(), [this](Team* a, Team* b) {
            return compareTeams(*a, *b);
        });
        
        for (auto* team : sortedTeams) {
            cout << team->name << " " << team->ranking << " " 
                 << team->solvedCount << " " << team->penaltyTime;
            
            for (auto& pname : problemNames) {
                cout << " ";
                auto it = team->problems.find(pname);
                if (it == team->problems.end() || (!it->second.solved && it->second.wrongAttempts == 0 && it->second.frozenSubmissions == 0)) {
                    cout << ".";
                } else if (it->second.solved) {
                    if (it->second.wrongAttempts == 0) {
                        cout << "+";
                    } else {
                        cout << "+" << it->second.wrongAttempts;
                    }
                } else if (it->second.frozenSubmissions > 0) {
                    if (it->second.wrongBeforeFreeze == 0) {
                        cout << "0/" << it->second.frozenSubmissions;
                    } else {
                        cout << "-" << it->second.wrongBeforeFreeze << "/" << it->second.frozenSubmissions;
                    }
                } else {
                    cout << "-" << it->second.wrongAttempts;
                }
            }
            cout << "\n";
        }
    }
    
public:
    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        Team team;
        team.name = name;
        teams[name] = team;
        teamOrder.push_back(name);
        cout << "[Info]Add successfully.\n";
    }
    
    void start(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        started = true;
        durationTime = duration;
        problemCount = problems;

        for (int i = 0; i < problems; i++) {
            problemNames.push_back(string(1, 'A' + i));
        }

        for (auto& name : teamOrder) {
            for (auto& pname : problemNames) {
                teams[name].problems[pname] = ProblemStatus();
            }
        }

        // Initialize rankings based on lexicographic order
        updateRankings();

        cout << "[Info]Competition starts.\n";
    }
    
    void submit(const string& problem, const string& teamName, const string& status, int time) {
        Team& team = teams[teamName];
        ProblemStatus& ps = team.problems[problem];

        Submission sub = {problem, status, time};
        ps.allSubmissions.push_back(sub);
        team.allSubmissions.push_back(sub);  // Also add to team's global submission list

        if (frozen && !ps.solved) {
            if (ps.frozenSubmissions == 0) {
                team.frozenProblemCount++;
            }
            ps.frozenSubmissions++;
            ps.frozenSubs.push_back(sub);
            teamsWithFrozenProblems.insert(teamName);
        } else if (!ps.solved) {
            if (status == "Accepted") {
                ps.solved = true;
                ps.solveTime = time;
            } else {
                ps.wrongAttempts++;
            }
        }
    }
    
    void flush() {
        for (auto& name : teamOrder) {
            calculateTeamStats(teams[name], false);
        }
        updateRankings();
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        teamsWithFrozenProblems.clear();

        for (auto& name : teamOrder) {
            teams[name].frozenProblemCount = 0;
            for (auto& p : teams[name].problems) {
                if (!p.second.solved) {
                    p.second.wrongBeforeFreeze = p.second.wrongAttempts;
                    p.second.frozenSubmissions = 0;
                    p.second.frozenSubs.clear();
                }
            }
        }

        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // Flush without printing the message
        for (auto& name : teamOrder) {
            calculateTeamStats(teams[name], false);
        }
        updateRankings();

        printScoreboard();

        // Build a sorted list of teams by current ranking
        vector<Team*> sortedTeams;
        for (auto& name : teamOrder) {
            sortedTeams.push_back(&teams[name]);
        }

        auto teamComparator = [this](Team* a, Team* b) {
            return compareTeams(*a, *b);
        };

        sort(sortedTeams.begin(), sortedTeams.end(), teamComparator);

        // Assign rankings based on sorted order
        for (size_t i = 0; i < sortedTeams.size(); i++) {
            sortedTeams[i]->ranking = i + 1;
        }

        // Unfreeze logic
        while (!teamsWithFrozenProblems.empty()) {
            // Find lowest ranked team with frozen problems
            Team* lowestTeam = nullptr;
            int lowestIdx = -1;

            for (int i = sortedTeams.size() - 1; i >= 0; i--) {
                Team* team = sortedTeams[i];
                if (teamsWithFrozenProblems.count(team->name) > 0) {
                    lowestTeam = team;
                    lowestIdx = i;
                    break;
                }
            }

            if (!lowestTeam) break;

            // Find smallest problem number with frozen submissions
            string smallestProblem = "";
            for (auto& pname : problemNames) {
                if (lowestTeam->problems[pname].frozenSubmissions > 0) {
                    smallestProblem = pname;
                    break;
                }
            }

            if (smallestProblem.empty()) continue;

            // Unfreeze this problem
            ProblemStatus& ps = lowestTeam->problems[smallestProblem];

            // Process frozen submissions for this problem
            for (auto& sub : ps.frozenSubs) {
                if (!ps.solved) {
                    if (sub.status == "Accepted") {
                        ps.solved = true;
                        ps.solveTime = sub.time;
                    } else {
                        ps.wrongAttempts++;
                    }
                }
            }

            ps.frozenSubmissions = 0;
            ps.frozenSubs.clear();

            // Update frozen problem count
            lowestTeam->frozenProblemCount--;
            if (lowestTeam->frozenProblemCount == 0) {
                teamsWithFrozenProblems.erase(lowestTeam->name);
            }

            calculateTeamStats(*lowestTeam, false);

            // Find new position using binary search
            int left = 0, right = lowestIdx;
            int newIdx = lowestIdx;
            while (left < right) {
                int mid = (left + right) / 2;
                if (compareTeams(*lowestTeam, *sortedTeams[mid])) {
                    right = mid;
                } else {
                    left = mid + 1;
                }
            }
            newIdx = left;

            if (newIdx < lowestIdx) {
                // Output ranking changes
                string replacedTeamName = sortedTeams[newIdx]->name;
                cout << lowestTeam->name << " " << replacedTeamName << " "
                     << lowestTeam->solvedCount << " " << lowestTeam->penaltyTime << "\n";

                // Move team to new position using rotate
                rotate(sortedTeams.begin() + newIdx, sortedTeams.begin() + lowestIdx, sortedTeams.begin() + lowestIdx + 1);

                // Update rankings
                for (int i = newIdx; i <= lowestIdx; i++) {
                    sortedTeams[i]->ranking = i + 1;
                }
            }
        }

        printScoreboard();
        frozen = false;
    }
    
    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        cout << teamName << " NOW AT RANKING " << teams[teamName].ranking << "\n";
    }
    
    void querySubmission(const string& teamName, const string& problem, const string& status) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        Team& team = teams[teamName];
        Submission* lastSub = nullptr;

        // Iterate through all submissions in reverse order to find the last matching one
        for (int i = team.allSubmissions.size() - 1; i >= 0; i--) {
            auto& sub = team.allSubmissions[i];
            if ((problem == "ALL" || sub.problem == problem) &&
                (status == "ALL" || sub.status == status)) {
                lastSub = &sub;
                break;
            }
        }

        if (!lastSub) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << teamName << " " << lastSub->problem << " "
                 << lastSub->status << " " << lastSub->time << "\n";
        }
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string name;
            iss >> name;
            system.addTeam(name);
        } else if (cmd == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            system.start(duration, problems);
        } else if (cmd == "SUBMIT") {
            string problem, by, teamName, with, status, at;
            int time;
            iss >> problem >> by >> teamName >> with >> status >> at >> time;
            system.submit(problem, teamName, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            system.queryRanking(teamName);
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, part1, part2;
            iss >> teamName >> where;
            getline(iss, part1);
            
            size_t probPos = part1.find("PROBLEM=");
            size_t statPos = part1.find("STATUS=");
            
            string problem = part1.substr(probPos + 8, part1.find(" AND") - probPos - 8);
            string status = part1.substr(statPos + 7);
            
            system.querySubmission(teamName, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}

