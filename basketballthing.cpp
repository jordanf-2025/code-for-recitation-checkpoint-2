#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

// Function to handle the data coming back from the API
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to talk to the NBA API
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

    // 1. Get Games
    auto gameJson = json::parse(apiRequest("https://api.balldontlie.io/v1/games?dates[]=" + dateChoice, apiKey));
    auto games = gameJson["data"];
    if (games.empty()) { std::cout << "No games found for this date.\n"; return 0; }

    std::cout << "\n--- Select a Game ---\n";
    for (int i = 0; i < (int)games.size(); i++) {
        std::cout << i + 1 << ") " << games[i]["visitor_team"]["abbreviation"] 
                  << " @ " << games[i]["home_team"]["abbreviation"] << std::endl;
    }

    int gameChoice;
    std::cout << "\nSelect game number: ";
    std::cin >> gameChoice;
    int selectedId = games[gameChoice - 1]["id"];

    // 2. Get Stats for that Game
    auto stats = json::parse(apiRequest("https://api.balldontlie.io/v1/stats?per_page=100&game_ids[]=" + std::to_string(selectedId), apiKey))["data"];
    
    std::string filename = "nba_stats_export.csv";
    std::ofstream file(filename);
    
    // CSV Header for Excel
    file << "Player,Team,PTS,REB,AST,MIN,STL,BLK\n";

    int count = 0;
    for (auto& s : stats) {
        if (count >= 50) break;

        // Extracting values safely
        std::string mins = s.value("min", "");
        int pts = s.value("pts", 0);
        int reb = s.value("reb", 0);
        int ast = s.value("ast", 0);

        // THE FILTER: Skip if they didn't play (checking multiple indicators)
        if ((mins == "" || mins == "0" || mins == "00" || mins == "0:00") && pts == 0 && reb == 0) {
            continue;
        }

        // Write row to CSV
        file << s["player"].value("first_name", "") << " " << s["player"].value("last_name", "") << ","
             << s["team"]["abbreviation"] << ","
             << pts << ","
             << reb << ","
             << ast << ","
             << mins << ","
             << s.value("stl", 0) << ","
             << s.value("blk", 0) << "\n";
        
        count++;
    }
    file.close();

    // 3. Launch Excel
    if (count > 0) {
        std::cout << "\nFound " << count << " active players. Opening in Excel..." << std::endl;
        #ifdef _WIN32
            system(("start excel " + filename).c_str());
        #else
            // Try Excel first; if fails, standard open
            system(("open -a 'Microsoft Excel' " + filename + " || open " + filename).c_str());
        #endif
    } else {
        std::cout << "No active player data found for this specific game." << std::endl;
    }

    return 0;
}
