#include <thread>
#include <algorithm>
#include <cstring>
#include <queue> 
#include <iostream>
#include <mutex>

class Word
{
public:
    Word(const char* data_) :
        data(::strdup(data_))
    {}

    Word() :
        data((char*)"")
    {}
	
	char * get_data() const
	{
		return data;
	}
	int get_count() const
	{
		return count;
	}
	void increment_count()
	{
		++count;
	}
	
private:
    char* data;
    int count = 0;
	
};

//Custom sort based on data entered  
struct {
    bool operator()(Word* word1, Word* word2) const
    {
        return std::strcmp(word1->get_data(), word2->get_data()) < 0;
    }
} customSort;

static std::queue<Word*> inputQueue;
static std::vector<Word*> s_wordsArray;
static int s_totalFound;
std::mutex dataPoolMutex;

/**
 * processWord	Process the user input while removing duplicates and insert them
 * 				in the 'word list' (s_wordsArray)
 *
 * @param[in] wordInput Word entered by the user.
 * @return void
 */
static void processWord(Word* wordInput)
{
    bool found = false;

    // Do not insert duplicate words
    for (auto p : s_wordsArray)
    {

        if (!std::strcmp(p->get_data(), wordInput->get_data()))
        {
            p->increment_count();
            found = true;
            break;
        }
    }

    if (!found)
    {
        wordInput->increment_count();
        s_wordsArray.push_back(wordInput);
    }
    else
    {
        //De allocate memory for rejected words.
        free(wordInput->get_data());
        free(wordInput);
    }
    found = false; //Reset the flag for next word
}

/**
 * workerThread	DeQueue the user input from the input queue, and process it.
 * 				Terminate when the word 'end' is encountered.
 *
 * @param None
 * @return void
 */
static void workerThread()
{
    bool endEncountered = false;
    bool qEmpty = true;
    Word* w;
    while (!endEncountered)
    {
        {
            std::lock_guard<std::mutex> lck{ dataPoolMutex };
            qEmpty = inputQueue.empty();
        }

        if (!qEmpty) // Do we have a new word?
        {
            {
                std::lock_guard<std::mutex> lck{ dataPoolMutex };
                w = inputQueue.front(); // Copy the word
                inputQueue.pop();
            }

            endEncountered = std::strcmp(w->get_data(), "end") == 0;

            if (!endEncountered)
            {
                processWord(w);
            }
        }
    }
};

/**
 * processInput add the words to the inputQueue,used by worker thread for
 * 				inclusion in the word list.
 *
 * @param[in] inputWord Word to be added.
 * @return void
 */
static bool processInput(std::string inputWord)
{
    bool endFound = false;
    std::size_t pos = inputWord.find(" ");      // filter the first word, until space in case of multiple words

    if (pos < 0)
    {
        pos = inputWord.length();
    }
    std::string word = inputWord.substr(0, pos);

    if (word.length() != 0)
    {
        if ((word.compare("end")) == 0)
        {
            endFound = true;
        }
        //Lock the queue
        {
            std::lock_guard<std::mutex> lck{ dataPoolMutex };
            Word* w = new Word((const char*)word.c_str()); // Copy the word
            inputQueue.push(w);
        }
    }

    return endFound;
}


/**
 * readInputWords 	Read input words from STDIN and process it
 *
 * @param None
 * @return void
 */
static void readInputWords()
{
    bool endEncountered = false;

    std::thread* worker = new std::thread(workerThread);

    std::string userInput;
    std::printf("\nEnter the list of words : \n");

    while (!endEncountered)
    {
        if (!std::getline(std::cin, userInput))
        {
            return;
        }
        else
        {
            endEncountered = processInput(userInput);
        }
    }

    worker->join(); // Wait for the worker to terminate
}

/**
 * searchInput User input word will be searched from the stored array.
 *
 * @param[in] searchWord Word to be searched.
 * @return void
 */
static void searchInput(std::string searchWord)
{
    bool found = false; //Initialize flag as 'Not found'
    Word* w = new Word(searchWord.c_str());

    // Search for the word
    unsigned i;

    for (i = 0; i < s_wordsArray.size(); ++i)
    {
        if (std::strcmp(s_wordsArray[i]->get_data(), w->get_data()) == 0)
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        std::printf("SUCCESS: '%s' was present %d times in the initial word list\n",
            s_wordsArray[i]->get_data(), s_wordsArray[i]->get_count());
        ++s_totalFound;
    }
    else
    {
        std::printf("'%s' was NOT found in the initial word list\n", w->get_data());
    }
    found = false;//Reset the flag
    free(w); //Clear the allocated memory
}

/**
 * lookupWords Repeatedly ask the user for a word and check whether it was present in the word list
 *			   Terminate on EOF
 * @param None
 * @return void
 */
static void lookupWords()
{
    char* linebuf = new char[32];
    std::string userSearchInput;

    for (;;)
    {
        std::printf("\nEnter a word for lookup:");
        if (!std::getline(std::cin, userSearchInput))
        {
            return;
        }

        // Initialize the word to search for
        std::size_t pos = userSearchInput.find(" ");      // position of "live" in str
        if (pos < 0)
        {
            pos = userSearchInput.length();
        }
        std::string word = userSearchInput.substr(0, pos);
        if (word.length() > 0)
        {
            searchInput(word);
        }
    }
}

int main()
{
    try
    {
        readInputWords();

        // Sort the words alphabetically

        std::sort(s_wordsArray.begin(), s_wordsArray.end(), customSort);

        // Print the word list
        std::printf("\n=== Word list:\n");

        for (auto p : s_wordsArray)
        {
            std::printf("%s %d\n", p->get_data(), p->get_count());
        }

        lookupWords();

        printf("\n=== Total words found: %d\n", s_totalFound);
    }
    catch (std::exception& e)
    {
        std::printf("error %s\n", e.what());
    }

    return 0;
}
