#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Function to handle API data stream
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to make the actual API call
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

    // 1. Fetch Games for the date
    std::string gameData = apiRequest("https://api.balldontlie.io/v1/games?dates[]=" + dateChoice, apiKey);
    auto gameJson = json::parse(gameData);
    auto games = gameJson["data"];

    if (games.empty()) {
        std::cout << "No games found for that date." << std::endl;
        return 0;
    }

    std::vector<int> gameIds;
    std::cout << "\n--- GAMES ON " << dateChoice << " ---\n";
    for (int i = 0; i < games.size(); i++) {
        gameIds.push_back(games[i]["id"]);
        std::cout << i + 1 << ") " << games[i]["visitor_team"]["abbreviation"] 
                  << " @ " << games[i]["home_team"]["abbreviation"] << std::endl;
    }

    int choice;
    std::cout << "\nSelect game number for stats: ";
    std::cin >> choice;
    if (choice < 1 || choice > gameIds.size()) return 1;

    // 2. Fetch Box Score for selected game
    int selectedId = gameIds[choice - 1];
    std::string statsData = apiRequest("https://api.balldontlie.io/v1/stats?game_ids[]=" + std::to_string(selectedId), apiKey);
    auto playerStats = json::parse(statsData)["data"];

    // 3. Save to CSV
    std::string filename = "nba_stats.csv";
    std::ofstream file(filename);
    file << "sep=,\nPlayer,Team,MIN,PTS,REB,AST\n";

    for (auto& s : playerStats) {
        file << s["player"].value("first_name", "") << " " << s["player"].value("last_name", "") << ","
             << s["team"].value("abbreviation", "") << ","
             << s.value("min", "0") << ","
             << s.value("pts", 0) << ","
             << s.value("reb", 0) << ","
             << s.value("ast", 0) << "\n";
    }
    file.close();

    // 4. SMART OPEN (Detects Windows vs Mac)
    std::cout << "Opening Excel..." << std::endl;
#ifdef _WIN32
    std::string winCmd = "start excel " + filename;
    system(winCmd.c_str());
#elif __APPLE__
    std::string macCmd = "open -a \"Microsoft Excel\" " + filename + " || open " + filename;
    system(macCmd.c_str());
#endif

    return 0;
}
