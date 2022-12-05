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

        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 4, 4 });
        server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { -8, 3, 7 });

        ASSERT(server.FindTopDocuments("белый кот"s).size() == 2);
        ASSERT(server.FindTopDocuments("пушистый хвост"s).size() == 1);
        ASSERT(server.FindTopDocuments("танцующие котята"s).empty());
    }
}

void TestNegativeWordSupport() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "ночной перевал Дятлова"s, DocumentStatus::ACTUAL, { 1, 4, 4 });
        server.AddDocument(1, "поиск медведя в лесу"s, DocumentStatus::ACTUAL, { -8, 3, 7 });
        server.AddDocument(2, "ночная охота и рабылка , также дятел "s, DocumentStatus::ACTUAL, { -4, 5, 3 });

        ASSERT(server.FindTopDocuments("-рыбалка охота на кабана"s).size() == 1);
        ASSERT(server.FindTopDocuments("дятел в -лесу саду"s).size() == 1);
    }

}

void TestMatching() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "ночной перевал Дятлова"s, DocumentStatus::ACTUAL, { 1, 4, 4 });
        server.AddDocument(1, "поиск медведя в лесу"s, DocumentStatus::ACTUAL, { -8, 3, 7 });
        server.AddDocument(2, "ночная охота и рабылка , также слон Петя "s, DocumentStatus::ACTUAL, { -4, 5, 3 });

        const auto& [words_2, status_2] = server.MatchDocument("-слон дятел на дереве"s, 2);

        {
            const auto& [words_1, status_1] = server.MatchDocument("ночная охота"s, 2);
            const vector<string> test_result = { "ночная"s, "охота"s };
            ASSERT(test_result == words_1);
        }

        {
            const auto& [matched_words, status] = server.MatchDocument("ночной перевал -Дятлова"s, 0);
            const vector<string> test_result = {}; 
            ASSERT(test_result == matched_words);
        }

    }
}

void TestSortsByRelevance() {
    SearchServer server;
    server.AddDocument(0, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "пушистый пёс"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "собка с ошейником"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto found_documents = server.FindTopDocuments("пушистый кот"s);
        ASSERT(found_documents.size() == 2);
        for (size_t i = 1; i < found_documents.size(); i++) {
            ASSERT(found_documents[i - 1].relevance >= found_documents[i].relevance);
        }
    }
}

void TestForRatingCalculation() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "ночной перевал Дятлова"s, DocumentStatus::ACTUAL, { 1, 4, 4 });
        server.AddDocument(1, "поиск медведя в лесу"s, DocumentStatus::ACTUAL, { -8, 3, 7 });
        server.AddDocument(2, "ночная охота и рабылка , также слон Петя "s, DocumentStatus::ACTUAL, { -4, 5, 3 , 6});

        const auto& found_documents = server.FindTopDocuments("ночная охота на медведя у Дятлова в лесу");

        ASSERT(found_documents[0].rating == ( -8 +3 + 7) / 3);
        ASSERT(found_documents[1].rating == ( 1 + 4 + 4) / 3);
        ASSERT(found_documents[2].rating == ( -4 + 5 + 3 + 6) / 4);
    }
}

void TestSearchByPredicate() {
    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 4, 4 });
        server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::REMOVED, { -8, 3, 7 });
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 2, -9, 12 });
        server.AddDocument(3, "овальный ухоженный скворец евгений"s, DocumentStatus::BANNED, { 5, 14, -2 });

        const auto& found_documents_1 = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        const auto& found_documents_2 = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 3 == 1; });

        ASSERT(found_documents_1[0].id == 0);
        ASSERT(found_documents_1[1].id == 2);

        ASSERT(found_documents_2[0].id == 1);
    }
}

void TestSearchByStatus() {

    {
        SearchServer server;

        server.SetStopWords("и в на"s);

        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 4, 4 });
        server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::REMOVED, { -8, 3, 7 });
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 2, -9, 12 });
        server.AddDocument(3, "овальный ухоженный скворец евгений"s, DocumentStatus::BANNED, { 5, 14, -2 });

        ASSERT(server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; }).size() == 1);
        ASSERT(server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::REMOVED; }).size() == 1);
        ASSERT(server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; }).size() == 2);

    }

}

void TestCalculateRelevance() {
    SearchServer server;

    server.AddDocument(0, "белый кот с новым ошейником"s, DocumentStatus::ACTUAL, { 1, 4, 4 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { -8, 3, 7 });
    server.AddDocument(2, "пес хороший и большой"s, DocumentStatus::ACTUAL, { 2, -9, 12 });

    const auto found_docs = server.FindTopDocuments("пушистый хороший кот"s);

    double relevance_query = log((3 * 1.0) / 1) * (2.0 / 4.0) + log((3 * 1.0) / 2) * (1.0 / 4.0);

    ASSERT(abs(found_docs[0].relevance - relevance_query) < 1e-6);
}

void TestSearchServer() {

    TestExcludeStopWordsFromAddedDocumentContent();
    TestSearchForGivenWords();
    TestNegativeWordSupport();
    TestMatching();
    TestSortsByRelevance();
    TestForRatingCalculation();
    TestSearchByPredicate();
    TestSearchByStatus();
    TestCalculateRelevance();
}
