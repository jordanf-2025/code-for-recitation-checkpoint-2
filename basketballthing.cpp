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
    
    std::string filename = "nba_results.csv";
    std::ofstream file(filename);
    file << "Player,Team,PTS,REB,AST,MIN\n";

    int count = 0;
    for (auto& s : stats) {
        if (count >= 50) break;

        std::string mins = s.value("min", "0");
        int pts = s.value("pts", 0);

        // Robust filter: if minutes are essentially zero AND they scored zero, they didn't play.
        if ((mins == "0" || mins == "00" || mins == "0:00" || mins == "") && pts == 0) {
            continue; 
        }

        file << s["player"].value("first_name", "") << " " << s["player"].value("last_name", "") << ","
             << s["team"]["abbreviation"] << ","
             << pts << ","
             << s.value("reb", 0) << ","
             << s.value("ast", 0) << ","
             << mins << "\n";
        count++;
    }
    file.close();

    std::cout << "File saved. Launching..." << std::endl;

    // --- CROSS-PLATFORM OPEN COMMAND ---
#ifdef _WIN32
    system(("start excel " + filename).c_str()); // Windows
#else
    system(("open " + filename).c_str());        // macOS
#endif

    return 0;
}
