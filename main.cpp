#include "data_frame.hpp"

int main()
{
    // read csv
    auto df = DataFrame::read_csv("data/input.csv", true, ",", "\r\n", true);
    std::cout << std::endl;
    df.describe();
    
    // rename header
    df.rename_header({{"name_", "name"}, {"age_", "age"}, {"height_", "height"}, {"weight_", "weight"}, {"vegan_", "vegan"}});
    std::cout << std::endl;
    std::cout << "original" << std::endl;
    std::cout << df << std::endl;

    // filter vegan and age > 30
    df.filter([](const DataFrame& row) { return row["vegan"].as<bool>() && row["age"].as<int>() > 30; });
    std::cout << std::endl;
    std::cout << "filtered (vegan and age > 30)" << std::endl;
    std::cout << df << std::endl;

    df.sort_by("age", true);
    std::cout << std::endl;
    std::cout << "sorted by age" << std::endl;
    std::cout << df << std::endl;

    // update age all rows
    for (auto& row : df.rows())
    {
        row["age"] = row["age"].as<int>() + 1;
    }
    std::cout << std::endl;
    std::cout << "update (age increment 1)" << std::endl;
    std::cout << df << std::endl;

    // update height and weight of 0, 2, 4 rows
    df[{{"height", "weight"}, {0, 2, 4}}] = std::vector<std::vector<double>>{{170.0, 60.0}, {160.0, 50.0}, {180.0, 70.0}};
    std::cout << std::endl;
    std::cout << "positional updated" << std::endl;
    std::cout << df << std::endl;

    // save csv
    df.to_csv("data/output.csv");
}