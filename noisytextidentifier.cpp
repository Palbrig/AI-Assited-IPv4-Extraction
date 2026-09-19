#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Returns true if c is allowed to be part of an IPv4 candidate token.
bool isCandidateChar(char c)
{
    return (c >= '0' && c <= '9') || c == '.' || c == ':';
}


// Parses a decimal number starting at index.
//
// Rules enforced here:
// - Must contain at least one digit.
// - Cannot contain more than maxDigits digits.
// - Leading zeros are forbidden unless the number is exactly "0".
// - Value cannot exceed maxValue.
// - Value is accumulated manually, digit by digit.
//
// On success, index is left pointing at the first non-digit character.
bool parseNumber(const std::string& token,
                 std::size_t& index,
                 int maxDigits,
                 unsigned int maxValue,
                 unsigned int& value)
{
    if (index >= token.size() ||
        token[index] < '0' || token[index] > '9')
    {
        return false;
    }

    std::size_t start = index;
    value = 0;
    int digitCount = 0;

    while (index < token.size() &&
           token[index] >= '0' && token[index] <= '9')
    {
        // Too many digits.
        if (digitCount >= maxDigits)
        {
            return false;
        }

        unsigned int digit = token[index] - '0';

        // Manual decimal accumulation.
        value = value * 10 + digit;

        ++digitCount;
        ++index;
    }

    // Leading zero is only legal for the number 0 itself.
    // Examples:
    // "0"   -> valid
    // "00"  -> invalid
    // "01"  -> invalid
    if (digitCount > 1 && token[start] == '0')
    {
        return false;
    }

    if (value > maxValue)
    {
        return false;
    }

    return true;
}


// Validates one complete candidate token.
bool parseIPv4Candidate(const std::string& token,
                        unsigned long& address,
                        int& port)
{
    std::size_t index = 0;
    unsigned int octets[4];

    // Parse exactly four octets.
    for (int i = 0; i < 4; ++i)
    {
        if (!parseNumber(token, index, 3, 255, octets[i]))
        {
            return false;
        }

        // The first three octets must be followed by a period.
        if (i < 3)
        {
            if (index >= token.size() || token[index] != '.')
            {
                return false;
            }

            ++index; // Skip the period.
        }
    }

    // If we are at the end, there is no port.
    if (index == token.size())
    {
        port = -1;
    }
    else
    {
        // Anything after the IPv4 address must begin with exactly one colon.
        if (token[index] != ':')
        {
            return false;
        }

        ++index; // Skip the colon.

        unsigned int parsedPort;

        if (!parseNumber(token, index, 5, 65535, parsedPort))
        {
            return false;
        }

        // The port must consume the rest of the candidate.
        //
        // Rejects things like:
        // 192.168.1.1:80:90
        // 192.168.1.1:80.
        // 192.168.1.1:80:
        if (index != token.size())
        {
            return false;
        }

        port = static_cast<int>(parsedPort);
    }

    // Build the 32-bit IPv4 value.
    address =
        (static_cast<unsigned long>(octets[0]) << 24) |
        (static_cast<unsigned long>(octets[1]) << 16) |
        (static_cast<unsigned long>(octets[2]) << 8)  |
        static_cast<unsigned long>(octets[3]);

    return true;
}


bool extractIPv4(const std::string& str,
                 unsigned long& outAddress,
                 int& outPort)
{
    // Required failure/default values.
    outAddress = 0;
    outPort = -1;

    std::size_t index = 0;

    while (index < str.size())
    {
        // Ignore separators until we find a possible candidate.
        while (index < str.size() && !isCandidateChar(str[index]))
        {
            ++index;
        }

        if (index >= str.size())
        {
            break;
        }

        // Find the ENTIRE candidate token.
        //
        // This is important because we are not allowed to extract a valid
        // address from the middle of a larger invalid sequence.
        std::size_t start = index;

        while (index < str.size() && isCandidateChar(str[index]))
        {
            ++index;
        }

        std::string candidate = str.substr(start, index - start);

        unsigned long address;
        int port;

        if (parseIPv4Candidate(candidate, address, port))
        {
            outAddress = address;
            outPort = port;
            return true;
        }

        // If this entire candidate was invalid, keep searching after it.
    }

    // No valid candidate was found.
    outAddress = 0;
    outPort = -1;
    return false;
}

// Converts the 32-bit address back into dotted-decimal form
// so that it can easily be displayed to the user.
void printIPv4(unsigned long address)
{
    unsigned long octet1 = (address >> 24) & 255;
    unsigned long octet2 = (address >> 16) & 255;
    unsigned long octet3 = (address >> 8) & 255;
    unsigned long octet4 = address & 255;

    std::cout << octet1 << "."
              << octet2 << "."
              << octet3 << "."
              << octet4;
}


int main()
{
    char choice = 'Y';

    // Continue running until the user chooses N or n.
    while (choice == 'Y' || choice == 'y')
    {
        std::string fileName;

        std::cout << "Enter the name of the text file: ";
        std::getline(std::cin, fileName);

        // Attempt to open the file.
        std::ifstream inputFile(fileName);

        if (!inputFile.is_open())
        {
            std::cout << "\nError: Could not open file \""
                      << fileName << "\".\n";
            std::cout << "Make sure the file is in the program's "
                      << "working directory.\n";
        }
        else
        {
            // Read the entire file into one string.
            std::stringstream buffer;
            buffer << inputFile.rdbuf();

            std::string text = buffer.str();

            inputFile.close();

            unsigned long address = 0;
            int port = -1;

            std::cout << "\nSearching file...\n\n";

            if (extractIPv4(text, address, port))
            {
                std::cout << "Valid IPv4 address found: ";

                printIPv4(address);

                std::cout << "\n";

                std::cout << "32-bit address value: "
                          << address << "\n";

                if (port != -1)
                {
                    std::cout << "Port: "
                              << port << "\n";
                }
                else
                {
                    std::cout << "Port: None\n";
                }
            }
            else
            {
                std::cout << "No valid IPv4 address was found.\n";
            }
        }

        // Ask whether another file should be checked.
        while (true)
        {
            std::cout << "\nWould you like to check another file? "
                      << "(Y/N): ";

            std::string answer;
            std::getline(std::cin, answer);

            if (answer.length() == 1 &&
                (answer[0] == 'Y' || answer[0] == 'y'))
            {
                choice = answer[0];
                std::cout << "\n";
                break;
            }

            if (answer.length() == 1 &&
                (answer[0] == 'N' || answer[0] == 'n'))
            {
                choice = answer[0];
                break;
            }

            std::cout << "Invalid input. Please enter Y or N.\n";
        }
    }

    std::cout << "\nProgram closed.\n";

    return 0;
}