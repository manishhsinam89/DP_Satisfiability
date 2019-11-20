/*
 * Equivalence Checker for netlists
 *
 *
 * Name 1: Bharghavi Sankar
 * Matriculation Number 1: 394031
 *
 * Name 2: Manish Kalra
 * Matriculation Number 2: 389174
 */

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>

using namespace std;

typedef enum {
	AND, OR, INV, XOR, ZERO, ONE, UNDEFINED
} GateType;

typedef struct {
	GateType type;
	vector<int> nets;
} Gate;

typedef vector<Gate> GateList;

//const char *values[] = {"POS", "NEG"};
//vector<string> PosNeg(values, values+2);
//string status;


int netCount1, netCount2;
int monitorCnt = 1;
//int cnfApplied = 10;

vector<string> inputs1, outputs1, inputs2, outputs2;
map<string, int> map1, map2;
map<int, int> counterExample;
GateList gates1, gates2, mitergates, gate2Modified;
vector<int> netForcnf;
bool emptyClause = false;
int equivalenceChk = 0;
//int choseVar;


int readFile(string filename, int & netCount, vector<string> & inputs,
		vector<string> & outputs, map<string, int> & map, GateList & gates) {
	ifstream file(filename.c_str());
	if (!file.is_open()) {
		return -1;
	}
	string curLine;
	// net count
	getline(file, curLine);
	netCount = atoi(curLine.c_str());
	// inputs
	getline(file, curLine);
	stringstream inputsStream(curLine);
	string buf;
	while (inputsStream >> buf) {
		inputs.push_back(buf);
	}
	// outputs
	getline(file, curLine);
	stringstream outputsStream(curLine);
	while (outputsStream >> buf) {
		outputs.push_back(buf);
	}
	// mapping
	for (size_t i = 0; i < inputs1.size() + outputs1.size(); i++) {
		getline(file, curLine);
		stringstream mappingStream(curLine);
		mappingStream >> buf;
		int curNet = atoi(buf.c_str());
		mappingStream >> buf;
		map[buf] = curNet;
	}
	// empty line
	getline(file, curLine);
	if (curLine.length() > 1) {
		return -1;
	}
	// gates
	while (getline(file, curLine)) {
		stringstream gateStream(curLine);
		gateStream >> buf;
		Gate curGate;
		curGate.type = (buf == "and" ? AND : buf == "or" ? OR :
						buf == "inv" ? INV : buf == "xor" ? XOR :
						buf == "zero" ? ZERO : buf == "one" ? ONE : UNDEFINED);
		if (curGate.type == UNDEFINED) {
			return -1;
		}
		while (gateStream >> buf) {
			int curNet = atoi(buf.c_str());
			curGate.nets.push_back(curNet);
		}
		gates.push_back(curGate);
	}
	return 0;
}

int readFiles(string filename1, string filename2) {
	if (readFile(filename1, netCount1, inputs1, outputs1, map1, gates1) != 0) {
		return -1;
	}
	if (readFile(filename2, netCount2, inputs2, outputs2, map2, gates2) != 0) {
		return -1;
	}
	return 0;
}

// Prints internal data structure
void printData(int & netCount, vector<string> & inputs,
		vector<string> & outputs, map<string, int> & map, GateList & gates) {
	cout << "Net count: " << netCount << "\n\n";
	cout << "Inputs:\n";
	for (size_t i = 0; i < inputs.size(); i++) {
		cout << inputs[i] << "\n";
	}
	cout << "\n";
	cout << "Outputs:\n";
	for (size_t i = 0; i < outputs.size(); i++) {
		cout << outputs[i] << "\n";
	}
	cout << "\n";
	cout << "Mapping (input/output port to net number):\n";
	for (size_t i = 0; i < inputs.size(); i++) {
		cout << inputs[i] << " -> " << map[inputs[i]] << "\n";
	}
	for (size_t i = 0; i < outputs.size(); i++) {
		cout << outputs[i] << " -> " << map[outputs[i]] << "\n";
	}
	cout << "\n";
	cout << "Gates:\n";
	for (size_t i = 0; i < gates.size(); i++) {
		Gate & curGate = gates[i];
		cout
				<< (curGate.type == AND ? "AND" : curGate.type == OR ? "OR" :
					curGate.type == INV ? "INV" : curGate.type == XOR ? "XOR" :
					curGate.type == ZERO ? "ZERO" :
					curGate.type == ONE ? "ONE" : "ERROR");
		cout << ": ";
		for (size_t j = 0; j < curGate.nets.size(); j++) {
			cout << curGate.nets[j] << " ";
		}
		cout << "\n";
	}
	cout << "\n";
}

// Prints the internal data structure for netlist 1 or 2
void printDataForNetlist(int netlistNumber) {
	if (netlistNumber == 1) {
		printData(netCount1, inputs1, outputs1, map1, gates1);
	} else if (netlistNumber == 2) {
		printData(netCount2, inputs2, outputs2, map2, gates2);
	} else {
		cout << "Invalid netlist number " << netlistNumber
				<< " (must be 1 or 2)\n";
	}
}

/*
 * Function to build CNF for gateList
 * Generic function to build any gate into cnfs
 * @vector <vector <int> > cnf is a reference variable
 * gates -> to get any gates as input(gates1, gates2, mitergates,...)
 * miterchk -> if this is set to miter, a Clause is created for OR based on the number
 * of XOR outputs present
 *
 * netForcnf is a global variable: vector<int>, nets are pushed into this vector which
 * acts as a clause and then this netForcnf is pushed into the cnf vector
 */
void buildCNFForVectors(vector<vector<int> > & cnf,
	GateList &gates, string miterchk = "") {
	int input1, input2, output;
	for (size_t i = 0; i < gates.size(); i++) {
		Gate & curGate = gates[i];

		if (curGate.type == XOR) {
			input1 = curGate.nets[0];
			input2 = curGate.nets[1];
			output = curGate.nets[2];

			netForcnf.push_back(input1);
			netForcnf.push_back(input2);
			netForcnf.push_back(-output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(-input1);
			netForcnf.push_back(-input2);
			netForcnf.push_back(-output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(-input1);
			netForcnf.push_back(input2);
			netForcnf.push_back(output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(input1);
			netForcnf.push_back(-input2);
			netForcnf.push_back(output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

		}

		else if (curGate.type == AND) {
			input1 = curGate.nets[0];
			input2 = curGate.nets[1];
			output = curGate.nets[2];

			netForcnf.push_back(-input1);
			netForcnf.push_back(-input2);
			netForcnf.push_back(output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(input1);
			netForcnf.push_back(-output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(input2);
			netForcnf.push_back(-output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

		} else if (curGate.type == OR) {
			input1 = curGate.nets[0];
			input2 = curGate.nets[1];
			output = curGate.nets[2];

			netForcnf.push_back(-input1);
			netForcnf.push_back(output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(-input2);
			netForcnf.push_back(output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(input1);
			netForcnf.push_back(input2);
			netForcnf.push_back(-output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

		}

		else if (curGate.type == INV) {
			input1 = curGate.nets[0];
			output = curGate.nets[1];

			netForcnf.push_back(input1);
			netForcnf.push_back(output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

			netForcnf.push_back(-input1);
			netForcnf.push_back(-output);
			cnf.push_back(netForcnf);
			netForcnf.clear();

		}

	}

	//Checking the number of XORS in miter and adding OR characteristic Function

	if ((miterchk == "miter") && (mitergates.size() != 0)) {
		for (size_t j = 0; j < mitergates.size(); j++) {
			netForcnf.push_back(mitergates[j].nets[2]);
		}
		cnf.push_back(netForcnf);
		netForcnf.clear();
	}

}

/*
 * Function to Equalize inputs
 * cnf is a reference variable -> any modification will effect on the variable used
 * to invoke this function
 */
void equalizeInputs(vector<vector<int> > &cnf) {
	// Equalizing inputs
	for (size_t i = 0; i < inputs1.size(); i++) {
		netForcnf.push_back(-(map1[inputs1[i]]));
		netForcnf.push_back(map2[inputs1[i]] + netCount1);
		cnf.push_back(netForcnf);
		netForcnf.clear();

		netForcnf.push_back(map1[inputs1[i]]);
		netForcnf.push_back(-(map2[inputs1[i]] + netCount1));
		cnf.push_back(netForcnf);
		netForcnf.clear();
	}

}

//Function to modify the netnumbers of Gate2 to avoid mismatch
void changeMyNetListNumber() {
	for (size_t i = 0; i < gates2.size(); i++) {
		Gate & gateRef = gates2[i];
		gateRef.type = gates2[i].type;
		for (size_t j = 0; j < gates2[i].nets.size(); j++) {
			gateRef.nets[j] = gates2[i].nets[j] + netCount1;
		}
		gate2Modified.push_back(gateRef);

	}
}

//Fuction to add the miter gates
void checkOutputsandAddMiterGates() {
	int count = netCount1 + netCount2 +1;

	for (size_t i = 0; i < outputs1.size(); i++) {
		Gate miterGateRef;
		miterGateRef.type = XOR;

		miterGateRef.nets.push_back(map1[outputs1[i]]);
		miterGateRef.nets.push_back(map2[outputs1[i]] + netCount1);
		miterGateRef.nets.push_back(count+i);

		mitergates.push_back(miterGateRef);
	}

}

//Function to print the cnf

void printCNF(vector<vector<int> > cnf) {
	cout << "[";
	for (size_t i = 0; i < cnf.size(); i++) {
		cout << "(";
		for (size_t j = 0; j < cnf[i].size(); j++) {
			cout << cnf[i][j] << ",";
		}
		cout << "),";

	}
	cout << "]" << "\n";
}

/*
 * Function to implement Unit CLause Rule
 */
void applyUnitClauseRule(vector <vector <int> > &cnf) {
	vector<int>::iterator itpos;
	int literal;

	for (size_t iterateCnfClause = 0; iterateCnfClause < cnf.size();
			iterateCnfClause++) {
		if (cnf[iterateCnfClause].size() == 1) { // Applying Unit clause rule


			literal = cnf[iterateCnfClause][0];
			//cout<<"unit clause "<< literal << "\n";
			if(literal > 0){
				counterExample[literal] = 1;

			}else if(literal<0){
				counterExample[-literal] = 0;

			}
			//cnf.erase(cnf.begin() + iterateCnfClause); //Removing the clause
			for (size_t i = 0; i < cnf.size(); i++) {
				for (size_t j = 0; j < cnf[i].size(); j++) {
					if(cnf[i][j] == literal) {
						cnf.erase(cnf.begin() + i); //If literal has positive value, set 1 in the position and whole clause becomes 1---so removing it
						i = i-1;
						iterateCnfClause = iterateCnfClause -1;
						break;
					}
					if (cnf[i][j] == (-literal)) { // Literal to be replaced with 0 is Found
						cnf[i].erase(cnf[i].begin() + j);
						j=j-1;
						break;
					}
				}
			}
			//monitorCnt -=1;
		}
	}
	//printCNF(cnf);

	//if(monitorCnt <= 1) {
		//applyUnitClauseRule(cnf);
	//} else {
		//monitorCnt += 1;
	//}
}


/*
 * Function to check for empty Clause in the cnf
 */
bool checkForEmptyClause(vector<vector<int> > cnf) {
	bool returnVal;
	for (size_t clause = 0; clause < cnf.size(); clause++) {
		if(cnf[clause].size() == 0) {
			returnVal = true;
		}
	}

	return returnVal;
}

/*
 * Function to access the element in map
 */
void checkCounterExample() {
	for (map<int,int>::iterator it=counterExample.begin(); it!=counterExample.end(); ++it)
		cout << it->first << "-->" << it->second << "\n";
}

/*
 * Function to print the counterExample
 */
void printCounterExample() {
	cout << "---Counter Example----- \n Inputs: \n";
	for(size_t i = 0; i < inputs1.size();i++) {
		cout<< inputs1[i] << ":" << counterExample[map1[inputs1[i]]] << "\n";
	}
	cout << "Outputs netlist 1: \n";
	for(size_t i = 0; i < outputs1.size();i++) {
		cout<< outputs1[i] << ":" << counterExample[map1[outputs1[i]]] << "\n";
	}
	cout << "Outputs netlist 2: \n";
	for(size_t i = 0; i < outputs2.size();i++) {
		cout<< outputs2[i] << ":" << counterExample[map2[outputs2[i]]+netCount1] << "\n";
	}
}


/*
 * Function to fetch the last clause and last element to chose as variable
 * returned to the invoking function for applying the cnf0or 1
 */

int chooseVariableToApplyCnf(vector <vector <int> > cnf) {
	return abs(cnf.back().back());
}
/*
 * Function to Apply cnf0 or 1
 */
vector <vector <int> > applyCnf0Or1(int ChosenVar, int ZeroOrOne, vector <vector <int> > cnf) {
	if(ZeroOrOne == 1){
		counterExample[ChosenVar] = 1;

		for (size_t clause = 0; clause < cnf.size(); clause++) {

				for (size_t lit = 0; lit < cnf[clause].size(); lit++) {
					//finding the chosen variable
					if (ChosenVar == cnf[clause][lit]) { // ChosenVar to be replaced is Found
						cnf.erase(cnf.begin() + clause); //If literal has positive value, set 1 in the position and whole clause becomes 1---so removing it
						clause =clause-1;
						break;
					} if (ChosenVar == -(cnf[clause][lit])) { // Literal to be replaced with 0 is Found
						cnf[clause].erase(cnf[clause].begin() + lit); //If literal has positive value, set 1 in the position and whole clause becomes 1---so removing it
						lit = lit-1;
						break;
					}
				}
			}
	} else if(ZeroOrOne == 0) {
		 counterExample[ChosenVar] = 0;


		for (size_t clause = 0; clause < cnf.size(); clause++) {
				for (size_t lit = 0; lit < cnf[clause].size(); lit++) {
					//finding the chosen variable
					if (ChosenVar == cnf[clause][lit]) { // ChosenVar to be replaced is Found
						cnf[clause].erase(cnf[clause].begin() + lit); //If literal has positive value, set 1 in the position and whole clause becomes 1---so removing it
						lit = lit-1;
						break;
					} if (ChosenVar == -(cnf[clause][lit])) { // Literal to be replaced with 0 is Found
						cnf.erase(cnf.begin() + clause); //If literal has positive value, set 1 in the position and whole clause becomes 1---so removing it
						clause = clause-1;
						break;
					}
				}
			}
	}

	return cnf;

}





//Function to implement Davis putnam algorithm
void DP(vector<vector<int> > cnf) {
	//vector <vector <int> > cnf0,cnf1;
	applyUnitClauseRule(cnf);
	emptyClause = checkForEmptyClause(cnf);
	//printCNF(cnf);
	if(cnf.size() == 0) {
		//Empty cnf
		//cout<< "Insideempty cnf \n";
		equivalenceChk = 1;
		return;
	} //else {
		//Check for empty clause
//	for (size_t i = 0; i < cnf.size(); i++) {
//			if(cnf[i].size() == 0) {
//				cout<<"Empty clause found \n";
//				return;
//			}
//		}
	if(emptyClause) {
		return;
	}
			int choseVar = chooseVariableToApplyCnf(cnf);
			vector< vector<int> > cnf0 = applyCnf0Or1(choseVar, 0, cnf);
			DP(cnf0);
			if(equivalenceChk == 0) {// this condition is for the emptyCnf condition to halt if it returns to this point
				vector< vector<int> > cnf1 = applyCnf0Or1(choseVar, 1, cnf);

				DP(cnf1);
			}

}

//Main function begins here
int main(int argc, char ** argv) {
	if (argc != 3) {
		cerr << "Wrong argument count!\n";
		return -1;
	}

	if (readFiles(argv[1], argv[2]) != 0) {
		cerr << "Error while reading files!\n";
		return -1;
	}

	// The following global variables are now defined (examples are for file xor2.net):
	//
	// int netCount1, netCount2
	// - total number of nets in netlist 1 / 2
	// - e.g. netCount1 is 3
	//
	// vector<string> inputs1, outputs1, inputs2, outputs2
	// - names of inputs / outputs in netlist 1 / 2
	// - e.g. inputs1[0] contains "a_0"
	//
	// map<string, int> map1, map2
	// - mapping from input / output names to net numbers in netlist 1 / 2
	// - e.g. map1["a_0"] is 1, map1["b_0"] is 2, ...
	//
	// GateList gates1, gates2
	// - list (std::vector<Gate>) of all gates in netlist 1 / 2
	// - e.g.:
	//   - gates1[0].type is XOR
	//   - gates1[0].nets is std::vector<int> and contains three ints (one for each XOR port)
	//   - gates1[0].nets[0] is 1 (first XOR input port)
	//   - gates1[0].nets[1] is 2 (second XOR input port)
	//   - gates1[0].nets[2] is 3 (XOR output port)

	// Print out data structure - (for debugging)
	cout << "Netlist 1:\n==========\n";
	printDataForNetlist(1);
	cout << "\nNetlist 2:\n==========\n";
	printDataForNetlist(2);

	//
	// Add your code to build the CNF.
	// The CNF should be a vector of vectors of ints. Each "inner" vector represents one clause. The "outer" vector represents the whole CNF.
	//

	vector<vector<int> > cnf;

	buildCNFForVectors(cnf, gates1); // cnf for gates1
	changeMyNetListNumber(); // modifying gates2 net numbers and storing in gate2modified
	buildCNFForVectors(cnf, gate2Modified); // cnf for gates2modifed
	equalizeInputs(cnf); //cnf for equalised inputs
	checkOutputsandAddMiterGates(); // forming Mitergates of type GateList mitergates(global)
	buildCNFForVectors(cnf, mitergates, "miter"); // cnf for the formed mitergates
	//
	// Check CNF for satisfiability using the Davis Putnam algorithm
	//
	DP(cnf);
	if(equivalenceChk == 0) {
		cout<< "Equivalent!";
	} else {

		cout<< "Not Equivalent!";
		//checkCounterExample();
		printCounterExample();

	}
	//
	// Print result
	//
	// ...

	return 0;
}
