#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <Python.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include "mt19937ar.h"
#include "parameters.h"
#include "gges.h"
#include "grammar.h"
#include "individual.h"

/** @file automaticML.c
*  @brief Grammar based genetic program module
*  @details Part of the algorithm that implements the genectic program based on grammar.
*  This code is based on the library Libgges.
*/


typedef struct
{
    long seed;
    int dataSeed; 
    int internalCV; 
    bool evalTest;
    int nCores;
    int timeout;
    
}ExecParams;

/**
* @brief Function to evaluate the individuals.
* 
* @details Function that measure the fitness of the individual (algorithm),
* calling sklearn to evaluate each individual 
* It also test the best algorithm produced by the CFG-GP in the test set,
* calling sklearn to evaluate it
*
* @param algorithms
* @param dataTraining
* @param dataTest
* @param seed
* @param dataSeed - to control the training and validation resample
* @param internalCV - to control the number of folds in the internal cross-validation
* @param evalTest - a control parameter to evaluate the test set only in the end of the evolutionary process
* @return The fitness of the individual
*/
static char *evaluate_algorithms(int G, char *algorithms, char *dataTraining, char *dataTest, ExecParams exP){

    PyObject *pName, *pModule, *pDict, *pFunc, *pValue;
    char *individuals_fitness = malloc(1000);
    strcpy(individuals_fitness, "");
    
    /* To append the current path to sys.path in order to be 
     * able to load your python module (assuming it is located  
     * in the local directory tpot/):*/
    PyObject *sys = PyImport_ImportModule("sys");    
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyString_FromString("./recipe/"));
    Py_DECREF(sys);
    Py_DECREF(path);    

    // Build the name object
    pName = PyString_FromString("recipe");

    // Load the module object
    pModule = PyImport_Import(pName);

    // pDict is a borrowed reference 
    pDict = PyModule_GetDict(pModule);

    PyObject* pArgs = NULL;
    pFunc = NULL;



    //Make the evaluation of the GP individuals on the training data:
    if((dataTest == NULL) && (!exP.evalTest)){
        // pFunc is also a borrowed reference
        pFunc = PyDict_GetItemString(pDict, "evaluate_inds");
        pArgs = PyTuple_Pack(8, PyInt_FromLong(G),
                                      PyString_FromString(algorithms), 
                                      PyString_FromString(dataTraining),
                                      PyInt_FromLong(exP.seed),
                                      PyInt_FromLong(exP.dataSeed),
                                      PyInt_FromLong(exP.internalCV),
                                      PyInt_FromLong(exP.nCores),
                                      PyInt_FromLong(exP.timeout));
    //Test the resultant algorithm on the test data:
    }else if((dataTest != NULL) && (!exP.evalTest)){
        pFunc = PyDict_GetItemString(pDict, "evaluate_on_test");
        pArgs = PyTuple_Pack(8, PyInt_FromLong(G),
                                      PyString_FromString(algorithms), 
                                      PyString_FromString(dataTraining),
                                      PyString_FromString(dataTest),
                                      PyInt_FromLong(exP.seed),
                                      PyInt_FromLong(exP.dataSeed),
                                      PyInt_FromLong(exP.nCores),
                                      PyInt_FromLong(exP.timeout));

    }if((dataTest != NULL) && (exP.evalTest)){

        pFunc = PyDict_GetItemString(pDict, "test_algorithm");
        pArgs = PyTuple_Pack(5,       PyString_FromString(algorithms), 
                                      PyString_FromString(dataTraining), 
                                      PyString_FromString(dataTest),
                                      PyInt_FromLong(exP.seed),
                                      PyInt_FromLong(exP.dataSeed));
    }

    pValue = NULL;

    if (PyCallable_Check(pFunc)){
         pValue = PyObject_CallObject(pFunc, pArgs);
    } else {
         PyErr_Print();
    }
    
    if (pValue != NULL){
        strcpy(individuals_fitness, PyString_AsString(pValue));
        Py_DECREF(pValue);
    }

    // Clean up
    Py_DECREF(pModule);
    Py_DECREF(pName);


  return individuals_fitness;

}

/**
* @brief Concatenate all the individuals in a single string
* 
* @details Concatenate all the individuals in a single string that is sent to the
*   python script to evaluation
*
* @param indiviudals The population generated by the algorithm
* @param N The size of the population
* @return A string with all the pipelines
*/
static char *concatenate(struct gges_individual **indiviudals, int N){
    int i = 0;
    //Define the total lenght of semicolon, which is the size of population:
    int semicomma_lenght = N;
    int indiviudals_lenght = 0;
    //Measure the lengh of all the individuals to use in malloc:
    for (i = 0; i < N ; i++){
        indiviudals_lenght += strlen(indiviudals[i]->mapping->buffer);
    }
    //Define the size of the individuals
    char *result = malloc(semicomma_lenght + indiviudals_lenght + 1);    
    i = 0;
    strcpy(result, "");
    //Concatenate the individuals, using semicolons to distinguish:
    while (i < N){        
        strcat(result, indiviudals[i]->mapping->buffer);
        strcat(result, ";");
        strcat(result, indiviudals[i+1]->mapping->buffer);

        if(i < N - 2){
            strcat(result, ";");
        }

        i = i + 2;
    }

    return result;
}


/**
* @brief Function to evaluate all the individuals
* 
* @details Function to evaluate all the individuals which were generated by the context-free grammar
*
* @param params The population generated by the algorithm
* @param G Generation number
* @param N The size of the population
* @param members The size of the population
* @param args
* @return A string with all the pipelines
*/
static  void eval(struct gges_parameters *params, int G, struct gges_individual **members, int N, void *args){
    int i = 0;
   
    //To resample the data each five generations, using the parameter dataSeed:
    if(G % 5 == 0){
    	params->dataSeed = params->dataSeed + 1;
    }

    ExecParams exP;  
    exP.seed=params->seed;
    exP.dataSeed=params->dataSeed; 
    exP.internalCV=params->internalCV; 
    exP.evalTest=false;
    exP.nCores=params->nCores;
    exP.timeout=params->timeout;
   
    //Concatenate the individuals in a single string:
    char *individuals = concatenate(members, N);
    //Measure the fitness of the individuals calling a python program (sklearn methods):
    char *results = evaluate_algorithms(G, individuals, params->dataTraining, NULL, exP);
    //Call to evaluate the individuals on the test set, to see the evolutionary curve in the test set:
    evaluate_algorithms(G, individuals, params->dataTraining, params->dataTest, exP);
    char *evaluation;
    evaluation = strtok (results,";");
    //Set the fitness of each individual according to python sklearn library:
    while (evaluation != NULL) {
        members[i]->fitness = atof(evaluation);
        evaluation = strtok(NULL, ";");
        i++;
    }
    
    //Just print the current results:
    for (i = 0; i < N ; i++){
        fprintf(stdout, "Individual: %s-->%f \n",members[i]->mapping->buffer, members[i]->fitness);
    }
}

/**
* @brief Function that gives a report about all the individuals
* 
* @details Function that gets run at the end of each generation, simply prints
* out the current generation, the worst fitness, the average fitness and the worst fitness.
* It also prints out the best individual* 
*
* @param params The parameters of the algorithm
* @param G Generation 
* @param stop_criterion To finish the final evaluation iff the best individual's criterion is reached
* @param N The size of the population
* @param members The population
* @param args
*/
static void report(struct gges_parameters *params, int G, bool stop_criterion,  struct gges_individual **members, int N, void *args){
    char stringSeed[10];
    double best = -1.00;
    double worst = 1.00;
    double average = 0;
    int i;
    char *testResult = malloc(1000);
    strcpy(testResult, "");

    FILE *results;

    ExecParams exP;  
    exP.seed=params->seed;
    exP.dataSeed=params->dataSeed; 
    exP.internalCV=params->internalCV; 
    exP.evalTest=true;
    exP.nCores=params->nCores;
    exP.timeout=params->timeout;

    snprintf(stringSeed, 10, "%ld", params->seed);

    char *resultFileName = malloc(strlen("results-")+strlen(stringSeed)+5);
    strcpy(resultFileName, "Results_");
    strcat(resultFileName, stringSeed);
    strcat(resultFileName, ".csv");


    results = fopen(resultFileName, "a+");

    
    best =  members[0]->fitness;
    worst = members[N-1]->fitness;

    for (i = 0; i < N; i++){    
        average +=  members[i]->fitness;  
    }
    average /= N;

    fprintf(stdout, "%3d %9.6f  %9.6f %9.6f [[ %s ]]\n", G, worst, average, best, members[0]->mapping->buffer);
    fprintf(stdout, "%s \n", "--------------------------------------------");
    
    //Save the reports in a file:
    if((G==params->generation_count) || (stop_criterion)){
        strcpy(testResult, evaluate_algorithms(G, members[0]->mapping->buffer, params->dataTraining, params->dataTest, exP));
        fprintf(results, "%s, %ld, %s\n", testResult, params->seed, members[0]->mapping->buffer);
        printf("Final result: %ld, %s, %s\n", params->seed, members[0]->mapping->buffer, testResult);
    }

    fclose(results);
    
    fflush(stdout);
}

/**
* @brief Main function of the algorithm
* 
* @details Main function that calls the libgges.
*
* @param argc
* @param argv
* @return EXIT_SUCCESS
*/
int main(int argc, char **argv){
    struct gges_population *pop;
    struct gges_parameters *params;
    struct gges_bnf_grammar *G;

    struct timeval t;
    gettimeofday(&t, NULL);


    params = gges_default_parameters();
    
    //Make the parse of the parameters which are set in a parameter file,
    //such as: number of generations, size of the population, cross-over and mutation probabilities, etc.
    //To see more about this argument, see the 'config/gecco2015-cfggp.ini' file
    parse_parameters(argv[1], params);
    
    //Setting the seed of the GP to control its randomic behaviour:
    params->seed = atoi(argv[2]);
    init_genrand(params->seed);

    params->rnd = genrand_real2;

    //includes the directory of the training and test datasets
    params->dataTraining = argv[3]; 
    params->dataTest = argv[4];
    params->nCores = atoi(argv[5]);
    params->timeout = atoi(argv[6]);
  
    //Load the grammar, which its grammar directory is defined by a parameter:
    G = gges_load_bnf(params->grammarDir);

    // Initialize the Python Interpreter:
    Py_Initialize();
    //Execute the grammar-based evolutonary system:
    pop = gges_run_system(params, G, eval, NULL, report, NULL);
    // Finish the Python Interpreter:
    Py_Finalize();


    //Clean up:
    gges_release_population(pop);
    free(params);
    gges_release_grammar(G);

    return EXIT_SUCCESS;
}
