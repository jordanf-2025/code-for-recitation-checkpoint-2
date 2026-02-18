#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

int minsToSeconds(std::string m) {
    if (m.empty() || m == "00" || m == "0") return 0;
    try {
        size_t pos = m.find(':');
        if (pos != std::string::npos) {
            int mins = std::stoi(m.substr(0, pos));
            int secs = std::stoi(m.substr(pos + 1));
            return (mins * 60) + secs;
        }
        return std::stoi(m) * 60;
    } catch (...) { return 0; }
}

struct PlayerStats {
    std::string name;
    std::string team;
    int pts, reb, ast, stl, blk, seconds;
    std::string minStr;
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string apiRequest(std::string url, std::string apiKey) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: " + apiKey).c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    return readBuffer;
}

int main() {
    std::string apiKey = "4367bfa4-b1bc-4af1-8685-eecf4601a4c6";
    std::string dateChoice;

    std::cout << "Enter a date (YYYY-MM-DD): ";
    std::cin >> dateChoice;

    auto gameJson = json::parse(apiRequest("https://api.balldontlie.io/v1/games?dates[]=" + dateChoice, apiKey));
    auto games = gameJson["data"];
    if (games.empty()) { std::cout << "No games found.\n"; return 0; }

    for (int i = 0; i < (int)games.size(); i++) {
        std::cout << i + 1 << ") " << games[i]["visitor_team"]["abbreviation"] << " @ " << games[i]["home_team"]["abbreviation"] << std::endl;
    }

    int gameChoice;
    std::cout << "\nSelect game number: ";
    std::cin >> gameChoice;
    int selectedId = games[gameChoice - 1]["id"];

    auto stats = json::parse(apiRequest("https://api.balldontlie.io/v1/stats?per_page=100&game_ids[]=" + std::to_string(selectedId), apiKey))["data"];
    
    std::vector<PlayerStats> playerList;
    for (auto& s : stats) {
        std::string mStr = s.value("min", "0");
        int p = s.value("pts", 0);
        if ((mStr == "" || mStr == "0" || mStr == "00" || mStr == "0:00") && p == 0) continue;

        PlayerStats ps;
        ps.name = s["player"].value("first_name", "") + " " + s["player"].value("last_name", "");
        ps.team = s["team"]["abbreviation"];
        ps.pts = p;
        ps.reb = s.value("reb", 0);
        ps.ast = s.value("ast", 0);
        ps.stl = s.value("stl", 0);
        ps.blk = s.value("blk", 0);
        ps.minStr = mStr;
        ps.seconds = minsToSeconds(mStr);
        playerList.push_back(ps);
    }

    // --- UPDATED TEAM + MINUTES SORTING ---
    std::sort(playerList.begin(), playerList.end(), [](const PlayerStats& a, const PlayerStats& b) {
        if (a.team != b.team) return a.team < b.team; // First sort by team name
        return a.seconds > b.seconds;               // Then sort by minutes (descending)
    });

    std::string filename = "nba_team_report.csv";
    std::ofstream file(filename);
    file << "Player,,Team,MIN,PTS,REB,AST,STL,BLK\n";

    std::string currentTeam = "";
    for (const auto& p : playerList) {
        // Add a blank row when the team changes to make it readable
        if (currentTeam != "" && currentTeam != p.team) {
            file << ",,,,,,,,\n"; 
        }
        currentTeam = p.team;

        file << p.name << ",," << p.team << "," << p.minStr << "," << p.pts << "," 
             << p.reb << "," << p.ast << "," << p.stl << "," << p.blk << "\n";
    }
    file.close();

    std::cout << "\nStats grouped by team and sorted by minutes. Opening..." << std::endl;
    #ifdef _WIN32
        system(("start excel " + filename).c_str());
    #else
        system(("open -a 'Microsoft Excel' " + filename + " || open " + filename).c_str());
    #endif

    return 0;
}
