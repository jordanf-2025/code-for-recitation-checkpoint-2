#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

    // 1. ASK FOR THE DATE
    std::cout << "Enter a date (YYYY-MM-DD) [Ex: 2026-02-12]: ";
    std::cin >> dateChoice;

    // 2. FETCH GAMES FOR THAT DATE
    std::string gameData = apiRequest("https://api.balldontlie.io/v1/games?dates[]=" + dateChoice, apiKey);
    auto gameJson = json::parse(gameData);
    auto games = gameJson["data"];

    if (games.empty()) {
        std::cout << "No games found for " << dateChoice << ". Try another date!" << std::endl;
        return 0;
    }

    std::vector<int> gameIds;
    std::cout << "\n--- GAMES ON " << dateChoice << " ---\n";
    for (int i = 0; i < games.size(); i++) {
        gameIds.push_back(games[i]["id"]);
        std::cout << i + 1 << ") " << games[i]["visitor_team"]["abbreviation"] 
                  << " @ " << games[i]["home_team"]["abbreviation"] 
                  << " (" << games[i].value("status", "TBD") << ")" << std::endl;
    }

    // 3. USER SELECTS GAME
    int choice;
    std::cout << "\nSelect a game number for full box score: ";
    std::cin >> choice;

    if (choice < 1 || choice > gameIds.size()) {
        std::cout << "Invalid choice." << std::endl;
        return 1;
    }

    int selectedId = gameIds[choice - 1];
    
    // 4. FETCH BOX SCORE
    std::cout << "Loading player stats..." << std::endl;
    std::string statsData = apiRequest("https://api.balldontlie.io/v1/stats?game_ids[]=" + std::to_string(selectedId), apiKey);
    auto statsJson = json::parse(statsData);
    auto playerStats = statsJson["data"];

    // 5. SAVE AND OPEN
    std::string filename = "nba_stats_" + dateChoice + ".csv";
    std::ofstream file(filename);
    file << "sep=,\nPlayer,Team,MIN,PTS,REB,AST,STL,BLK\n";

    for (auto& s : playerStats) {
        file << s["player"].value("first_name", "") << " " << s["player"].value("last_name", "") << ","
             << s["team"].value("abbreviation", "") << ","
             << s.value("min", "0") << ","
             << s.value("pts", 0) << ","
             << s.value("reb", 0) << ","
             << s.value("ast", 0) << ","
             << s.value("stl", 0) << ","
             << s.value("blk", 0) << "\n";
    }
    file.close();

    std::cout << "Opening Excel..." << std::endl;
    std::string cmd = "start excel " + filename;
    system(cmd.c_str());

    return 0;
}