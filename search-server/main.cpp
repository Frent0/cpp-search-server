
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }

    {
        SearchServer server;

        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestSearchForGivenWords() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1,4,4 });
        server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { -8,3,7 });

        ASSERT(server.FindTopDocuments("белый кот"s).size() == 2u);
        ASSERT(server.FindTopDocuments("пушистый хвост"s).size() == 1u);
        ASSERT(server.FindTopDocuments("танцующие котята"s).empty());
    }
}

void TestNegativeWordSupport() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "ночной перевал Дятлова"s, DocumentStatus::ACTUAL, { 1,4,4 });
        server.AddDocument(1, "поиск медведя в лесу"s, DocumentStatus::ACTUAL, { -8,3,7 });
        server.AddDocument(2, "ночная охота и рабылка , также дятел "s, DocumentStatus::ACTUAL, { -4,5,3 });

        ASSERT(server.FindTopDocuments("-рыбалка охота на кабана"s).size() == 1u);
        ASSERT(server.FindTopDocuments("дятел в -лесу саду"s).size() == 1u);
    }

}

void TestMatching() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "ночной перевал Дятлова"s, DocumentStatus::ACTUAL, { 1,4,4 });
        server.AddDocument(1, "поиск медведя в лесу"s, DocumentStatus::ACTUAL, { -8,3,7 });
        server.AddDocument(2, "ночная охота и рабылка , также слон Петя "s, DocumentStatus::ACTUAL, { -4,5,3 });

        const auto& [words_1, status_1] = server.MatchDocument("ночная охота"s, 2u);
        const auto& [words_2, status_2] = server.MatchDocument("-слон дятел на дереве"s, 2u);

        ASSERT(words_1.size() == 2u);
        ASSERT(words_2.empty());
    }
}

void TestSortByRelevance() {
    {
        SearchServer server;

        server.AddDocument(0, "пушистый кот с пушистым хвостом"s, DocumentStatus::ACTUAL, { 7, 8, 3 });
        server.AddDocument(1, "пушистая собака"s, DocumentStatus::ACTUAL, { -1, 6, -5 });
        server.AddDocument(2, "собачий поводок из кожи"s, DocumentStatus::ACTUAL, { 9, 12,-7 });

        const auto found_documents = server.FindTopDocuments("пушистый кот"s);

        ASSERT(found_documents.size() == 1u);

        for (size_t i = 1u; i < found_documents.size(); i++) {
            ASSERT(found_documents[i - 1u].relevance >= found_documents[i].relevance);
        }

    }
}

void TestForRatingCalculation() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "ночной перевал Дятлова"s, DocumentStatus::ACTUAL, { 1,4,4 });
        server.AddDocument(1, "поиск медведя в лесу"s, DocumentStatus::ACTUAL, { -8,3,7 });
        server.AddDocument(2, "ночная охота и рабылка , также слон Петя "s, DocumentStatus::ACTUAL, { -4,5,3 });

        const auto& found_documents = server.FindTopDocuments("ночная охота на медведя у Дятлова в лесу");

        ASSERT(found_documents[0].rating == 0u);
        ASSERT(found_documents[1].rating == 3u);
        ASSERT(found_documents[2].rating == 1u);
    }
}

void RequesFromTheUserAndSearchByTheSpecifiedStatus() {

    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1,4,4 });
        server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::REMOVED, { -8,3,7 });
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 2,-9,12 });
        server.AddDocument(3, "овальный ухоженный скворец евгений"s, DocumentStatus::BANNED, { 5,14,-2 });


        ASSERT(server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; }).size() == 2);
        ASSERT(server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; }).size() == 1);

    }

}

void TestCalculateRelevance() {
    SearchServer server;

    server.AddDocument(0, "белый кот с новым ошейником"s, DocumentStatus::ACTUAL, { 1,4,4 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { -8,3,7 });
    server.AddDocument(2, "пес хороший и большой"s, DocumentStatus::ACTUAL, {2, -9, 12});

    const auto found_docs = server.FindTopDocuments("пушистый хороший кот"s);

    double relevance_query = log((3 * 1.0) / 1) * (2.0 / 4.0) + log((3 * 1.0) / 2) * (1.0 / 4.0);

    {
        ASSERT(abs(found_docs[0].relevance - relevance_query) < 1e-6);
    }
}

void TestSearchServer() {

    TestExcludeStopWordsFromAddedDocumentContent();
    TestSearchForGivenWords();
    TestNegativeWordSupport();
    TestMatching();
    TestSortByRelevance();
    TestForRatingCalculation();
    RequesFromTheUserAndSearchByTheSpecifiedStatus();
    TestCalculateRelevance();
}
int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
}
