#include "WFC.h"

int main()
{
    //Dimensions for the whole level.
    uint32_t w = 30;
    uint32_t h = 15;
    uint32_t d = 30;

    //Number of levels to generate.
    uint32_t nrOfRooms = 8;

    //The generated space converges around these sizes. Per room.
    uint32_t maxWidth = 11;
    uint32_t maxHeight = 4;
    uint32_t maxDepth = 11;

    std::string input = "largerTest1Output_Floors";

    //Create a WFC interface and send the input.
    WFC* wfc = new WFC(w, h, d);

    //Change so that we can setinput per room.
    wfc->SetInput(input + ".txt");

    //The generation has a certain amount of chances to succeed.
    unsigned chances = 100;
    while (!wfc->GenerateLevel(nrOfRooms, maxWidth, maxHeight, maxDepth) && chances > 0)
    {
        chances--;
        std::cout << chances << std::endl;
    }
    if (chances != 0)
    {
        //Output the generated level to a textfile.
        std::vector<std::string> generatedLevel = wfc->GetGeneratedLevel();
        std::vector<Room> generatedRooms = wfc->GetGeneratedRoomsData();

        std::ofstream output;
        output.open("testRooms_generatedLevel.txt");

        //Write the data about the rooms
        for (auto& r : generatedRooms)
        {
            output << r.globalPos[0] << "," << r.globalPos[1] << "," << r.globalPos[2] << "," << r.width << "," << r.height << "," << r.depth << "\n";
        }

        output << "\n";

        //Write the level data.
        for (uint32_t i{ 0u }; i < d; ++i)
        {
            for (uint32_t j{ 0u }; j < h; ++j)
            {
                for (uint32_t k{ 0u }; k < w; ++k)
                {
                    output << generatedLevel[i * h * w + j * w + k] << " ";
                }
                output << "\n";
            }
            output << "-\n";
        }
        output.close();
    }
    else
    {
        std::cout << "OUT OF TRIES!" << std::endl;
    }
    
    delete wfc;
}