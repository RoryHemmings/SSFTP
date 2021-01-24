#include <logger.h>

using namespace std;

string ColorText(const string& message, LOGGER::COLOR color)
{
  string ret = "\033[" + to_string(color) + "m" + message + "\033[0m"; 
  return ret;
}

void LOGGER::Log(const string& message)
{
  LOGGER::Log(message, LOGGER::COLOR::WHITE);
}

void LOGGER::Log(const string& message, COLOR color, bool newLine)
{
  // cout << "[" << ColorText("info", color) << "]" << message << endl;
  if (newLine)
  {
    cout << ColorText(message, color) << endl;
  }
  else
  {
    cout << ColorText(message, color);
  }
  
}

void LOGGER::DebugLog(const string& message)
{
  cout << ColorText("[Debug] ", LOGGER::COLOR::MAGENTA) << ColorText(message, LOGGER::COLOR::WHITE) << endl;  
}

void LOGGER::LogError(const string& message)
{
  cout << ColorText("[Error] ", LOGGER::COLOR::RED) << ColorText(message, LOGGER::COLOR::RED) << endl;  
}

