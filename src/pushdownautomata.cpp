#include "include/pushdownautomata.h"
#include "include/utilities.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <regex>
#include <iomanip>



PushdownAutomata::PushdownAutomata(std::string programFilePath)
{
    std::string program;
    std::ifstream programFile;

    programFile.open(programFilePath);
    if(programFile.is_open()){
            std::stringstream strStream;
            strStream << programFile.rdbuf();
            program = strStream.str();
        }else{
            throw std::runtime_error("File " + programFilePath + " cannot be opened.\n");
        }
     programFile.close();

    removeComments(program);
    removeEmptyLines(program);

    std::istringstream programReader_(program);

    std::string statesIdentifiers;
    std::getline(programReader_, statesIdentifiers);
    createStates(statesIdentifiers);

    std::string tapeAlphabet;
    std::getline(programReader_, tapeAlphabet);
    mTape = AutomataTape(createAlphabet(tapeAlphabet));

    std::string stackAlphabet;
    std::getline(programReader_, stackAlphabet);

    std::string initialStateIdentifier;
    std::getline(programReader_, initialStateIdentifier);
    setInitialState(initialStateIdentifier);

    std::string initialStackSymbol;
    std::getline(programReader_, initialStackSymbol);
    mStack = AutomataStack(createAlphabet(stackAlphabet), static_cast<char>(initialStackSymbol[0]));


    std::string acceptationStatesIdentifiers;
    std::getline(programReader_, acceptationStatesIdentifiers);
    setAcceptationStates(acceptationStatesIdentifiers);

    std::string transitionFunction;
    while(std::getline(programReader_, transitionFunction)){
        setTransitionFunction(transitionFunction);
    }

}

void PushdownAutomata::removeComments(std::string &program)
{
    // Comments are deleted from the program.
    std::regex comments("#.*");
    program = std::regex_replace(program, comments, "");
}

void PushdownAutomata::removeEmptyLines(std::string &program)
{
    // Empty newlines are deleted from the program.
    std::string tempProgram;
    std::istringstream programReader(program);
    std::string line;
        while (std::getline(programReader, line)) {
            if(!line.empty()){
               tempProgram.append(line);
               tempProgram.append("\n");
            }
     }

        program = tempProgram;
}

void PushdownAutomata::createStates(std::string statesIdentifiersString)
{
    std::vector<std::string> statesIdentifiers = Utilities::splitString(statesIdentifiersString);
    for(auto stateIdentifier : statesIdentifiers){
        mStates.push_back(AutomataState(stateIdentifier));
    }
}

void PushdownAutomata::setInitialState(std::string initialState)
{
    for(int i = 0; i < mStates.size(); i++){
        if(mStates[i].identifier() == initialState){
            mInitialState = &mStates[i];
            mCurrentState = &mStates[i];
        }
    }
}

void PushdownAutomata::setAcceptationStates(std::string acceptationStatesIdentifiersString)
{
    std::vector<std::string> acceptationStatesIdentifiers = Utilities::splitString(acceptationStatesIdentifiersString);
    for(auto acceptationStateIdentifier : acceptationStatesIdentifiers){
        auto state = findState(acceptationStateIdentifier);
        state -> setAcceptationState(true);
    }
}

void PushdownAutomata::setTransitionFunction(std::string transitionFunctionString)
{
    std::vector<std::string> transitionFunction = Utilities::splitString(transitionFunctionString);
    std::string currentStateIdentifier = transitionFunction[0];
    char inputSymbol = static_cast<char>(transitionFunction[1][0]);
    char popSymbol = static_cast<char>(transitionFunction[2][0]);
    std::string nextStateIdentifier = transitionFunction[3];
    std::vector<char> pushSymbols;
    for(int i = 4; i < transitionFunction.size(); i++){
        pushSymbols.push_back(static_cast<char>(transitionFunction[i][0]));
    }

    TransitionFunction t(inputSymbol, popSymbol, pushSymbols, nextStateIdentifier);
    auto state = findState(currentStateIdentifier);
    state -> insertTransitionFunction(t);

}

std::set<char> PushdownAutomata::createAlphabet(std::string alphabetString)
{
    std::set<char> alphabet;
    std::vector<std::string> tempAlphabet = Utilities::splitString(alphabetString);
    for(auto symbol : tempAlphabet){
        alphabet.insert(static_cast<char>(symbol[0]));
    }
    return alphabet;
}

std::vector<AutomataState>::iterator PushdownAutomata::findState(std::string identifier)
{
    std::vector<AutomataState>::iterator iterator = mStates.begin();
    while(iterator != mStates.end()){
        if(iterator -> identifier() == identifier)
            break;
        else iterator += 1;
    }
    return iterator;
}


/*
 *  TODO: After testing all this functions should be erased.
*/

AutomataTape PushdownAutomata::tape() const
{
    return mTape;
}

void PushdownAutomata::setTape(const AutomataTape &tape)
{
    mTape = tape;
}

AutomataStack PushdownAutomata::stack() const
{
    return mStack;
}

void PushdownAutomata::setStack(const AutomataStack &stack)
{
    mStack = stack;
}

std::vector<AutomataState> PushdownAutomata::states() const
{
    return mStates;
}

void PushdownAutomata::setStates(const std::vector<AutomataState> &states)
{
    mStates = states;
}

AutomataState *PushdownAutomata::currentState() const
{
    return mCurrentState;
}

void PushdownAutomata::setCurrentState(AutomataState *currentState)
{
    mCurrentState = currentState;
}

AutomataState *PushdownAutomata::initialState() const
{
    return mInitialState;
}

// ---------------------------------------------------------


void PushdownAutomata::setInputString(std::string inputString)
{
    mTape.setInputString(inputString);
}

bool PushdownAutomata::isAccepted(std::string inputString, bool verbose)
{
    // In case that trash values are stored from previous
    // inputs.
    mStack.reset();
    Utilities::depthCounter = 0;

    setInputString(inputString);
    bool solutionFound = false;
    runMachine(mInitialState, mStack, solutionFound, 0, verbose);
    return solutionFound;
}


void PushdownAutomata::runMachine(AutomataState *state, AutomataStack currentStack, bool &solutionFound, int currentSymbol, bool& verbose)
{
    mCurrentState = state;
    if(state -> acceptationState()){
        if(currentSymbol == mTape.inputSize()){
            solutionFound = true;
            return;
        }
    }      

    
    std::vector<TransitionFunction> transitionFunctions = state -> transitionFunction(mTape.getSymbol(currentSymbol));

    for(auto transition : transitionFunctions){
        AutomataStack currentStackCopy(currentStack);
        if(!solutionFound){
            char popSymbol = transition.popSymbol();
            std::vector<char> pushSymbols = transition.pushSymbols();
            std::string nextState = transition.nextState();
            if(popSymbol == currentStackCopy.top()){

                if(verbose){
                    printMachineInformation(state -> identifier(), currentStack, transition);
                }

                currentStackCopy.pop();
                currentStackCopy.push(pushSymbols);

                if(transition.inputSymbol() != '.'){
                    Utilities::depthCounter += 1;
                    runMachine(&(*findState(nextState)), currentStackCopy, solutionFound, currentSymbol + 1, verbose);
                    Utilities::depthCounter -= 1;
                }else{
                    Utilities::depthCounter += 1;
                    runMachine(&(*findState(nextState)), currentStackCopy, solutionFound, currentSymbol, verbose);
                    Utilities::depthCounter -= 1;
                }

            }
         }
       }
}

void PushdownAutomata::printMachineInformation(std::string stateIdentifier, AutomataStack& currentStack, TransitionFunction& transition){
    std::cout << "\n";
    Utilities::tabulate(Utilities::depthCounter);
    std::cout << "STATE: " << stateIdentifier << "\n";
    Utilities::tabulate(Utilities::depthCounter);
    std::cout << "STACK : ";
    currentStack.printStack();
    Utilities::tabulate(Utilities::depthCounter);
    std::cout << stateIdentifier << ": ";
    transition.printTransitionFunction();
}
