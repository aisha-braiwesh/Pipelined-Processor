#include <bits/stdc++.h>
using namespace std;
vector<bool> stallCycle(1000,false);
int globalCycle=1;
vector<string>stages= {"IF","ID","EX","MA","WB"} ;
struct Instruction {
	string instr;
	string op,des,s1,s2;
	int optype ;//0 lw ,1 sw,2 R-format ,3 control
	int first_cycle=-1;
	int id,originalId;
	map<int,string>timeline;
	Instruction(string t):instr(t), optype(4), id(-1), originalId(-1) {
		parse();
	}

	void parse() {
		stringstream ss(instr);
		ss >> op;
		if((op)=="add"||(op)=="sub"||(op)=="and"||op=="or") {
			ss>>des>>s1>>s2;
			optype=2;
		} else if((op)=="lw") {
			ss>>des>>s1;

			optype=0;
		} else if((op)=="sw") {
			ss>>s1>>des;
			optype=1;
		}
	}

};

// ---------- Dependency Detection ----------
bool detectRAW(const Instruction& prev,const Instruction& curr) {
	if(prev.des.empty()) return false;
	bool find=(prev.des==curr.s1 || prev.des==curr.s2);
	return find;
}

bool detectWAW(const Instruction& prev,const Instruction& curr) {
	if(prev.des.empty() || curr.des.empty()) return false;
	return prev.des==curr.des;
}

bool detectWAR(const Instruction& prev,const Instruction& curr) {
	if(curr.des.empty()) return false;
	return (prev.s1==curr.des or prev.s2==curr.des);
}
//------------Print dependances------------
void printDependances(vector<Instruction>& inst) {
	cout<<"Data dependances :"<<endl;
	for(int i=0; i<inst.size(); i++) {
		for(int j=i+1; j<inst.size(); j++) {
			if(detectRAW(inst[i],inst[j]))cout<<"I"<<j<<" depends on I"<<i<<" (RAW)\n";
			if(detectWAW(inst[i],inst[j]))cout<<"I"<<j<<" depends on I"<<i<<" (WAW)\n";
			if(detectWAR(inst[i],inst[j]))cout<<"I"<<j<<" depends on I"<<i<<" (WAR)\n";

		}
	}
}
// ---------- Detect Forwarding ----------
string detectForwarding(const Instruction& curr,const Instruction& prev,const Instruction& pprev) {
	if(detectRAW(prev,curr)&&detectRAW(pprev,curr))
	{
		if(prev.optype==2 and pprev.optype==2)
		{
			return "stall stall ";
		}
		else if(prev.optype==2 and pprev.optype==0)
		{
			return "yes";
		}
		else if(prev.optype==0 and pprev.optype==2) {
			return "stall";
		}
		else if(prev.optype==0 and pprev.optype==0) {
			return "stall";
		}
	}
	if (detectRAW(prev,curr))
	{
		if(prev.optype==0) {
			return "stall";
		}
		else if(prev.optype==2) {
			return "yes";
		}
	}
	if (detectRAW(pprev,curr)) {
		if(pprev.optype==0) {
			return "yes";
		}
		else if(pprev.optype==2) {
			return "stall";
		}

	}
	return "yes";
}
//------------Forwarding-------------
void Forwarding( vector<Instruction>& inst) {
	for(int i=0; i<inst.size(); i++) {
		Instruction &curr=inst[i];
		while(stallCycle[globalCycle]) {
			curr.timeline[globalCycle]="stall";
			globalCycle++;
		}
		curr.first_cycle=globalCycle;
		curr.timeline[globalCycle]="IF";
		globalCycle++;
		int tempcycle=globalCycle;
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		curr.timeline[tempcycle]="ID";
		tempcycle++;
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		if(i==0) {
			curr.timeline[tempcycle]="EX";
			tempcycle++;
		}
		else if(i==1) {
			if(detectRAW(inst[0],inst[1])&&inst[0].optype==2) {
				curr.timeline[tempcycle]="EX";
				tempcycle++;
			}
			else if(detectRAW(inst[0],inst[1])&&inst[0].optype==0) {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="EX";
				tempcycle++;

			}
			else {
				curr.timeline[tempcycle]="EX";
				tempcycle++;
			}


		}
		else {
			if(detectForwarding(inst[i],inst[i-1],inst[i-2])=="yes") {
				curr.timeline[tempcycle]="EX";
				tempcycle++;
			}
			else if(detectForwarding(inst[i],inst[i-1],inst[i-2])=="stall") {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="EX";
				tempcycle++;
			}
			else {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="EX";
				tempcycle++;

			}
		}
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		curr.timeline[tempcycle]="MA";
		tempcycle++;
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		curr.timeline[tempcycle]="WB";
		tempcycle++;


	}
}

//----- range can move in------
void setdependances(vector<Instruction>&inst,vector<pair<int,int>>&rangecanmovein) {
	rangecanmovein=vector<pair<int,int>>(inst.size(), {-1,inst.size()});
	for(int i=0; i<inst.size(); i++) {
		for(int j=i-1; j>=0; j--) {
			if(detectRAW(inst[j],inst[i])||detectWAW(inst[j],inst[i])||detectWAR(inst[j],inst[i])) {
				rangecanmovein[i].first=j;
				break;
			}
		}
		for(int j=i+1; j<inst.size(); j++) {
			if(detectRAW(inst[i],inst[j])||detectWAW(inst[i],inst[j])||detectWAR(inst[i],inst[j])) {
				rangecanmovein[i].second=j;
				break;
			}
		}
	}
}
//-------- ReorderingWithForwarding ---------
void ReorderingWithForwarding( vector<Instruction>&inst) {
	for (int i=0 ; i<inst.size() ; i++) {
		if(inst[i].optype == 1)
		{
			int j=i+1;
			while(j<inst.size() and (inst[j].optype !=0 and inst[i].s1==inst[j].des)) {
				if(inst[j].optype == 0 and inst[i].des==inst[j].s1)
				{
					for(int k=j+1 ; k<inst.size() ; k++ ) {
						if(inst[k].des == inst[j].des)
						{
							inst[k].des=inst[i].s1 ;
						}
						if(inst[k].s1 == inst[j].des)
						{
							inst[k].s1=inst[i].s1 ;
						}
						if(inst[k].s2 == inst[j].des)
						{
							inst[k].s2=inst[i].s1 ;
						}
					}
					inst.erase(inst.begin()+j);
					j--;
				}
				j++;
			}

		}
	}

	bool done=false;
	vector<pair<int,int>>rangecanmovein(inst.size());
	setdependances(inst,rangecanmovein);
	if(detectRAW(inst[0],inst[1])&& inst[0].optype==0) {

		for(int i=2; i<inst.size(); i++)
		{	int c=i,p=1;
			if(rangecanmovein[i].first<=p-1 and !done) {
				if((c-1<0 or inst[c-1].optype!=0 or !detectRAW(inst[c-1],inst[p-1]))
				        and((c-2<0) or (inst[c-2].optype!=2) or (!detectRAW(inst[c-2],inst[p-1])))
				        and((c+1==inst.size() or (inst[c-1].optype==1))
				            or ((inst[c-1].optype==0) && !(detectRAW(inst[c-1],inst[c+1])))
				            or( inst[c-2].optype==2 && !(detectRAW(inst[c-2],inst[c+1])))))
				{
					if((c+2>=inst.size()) or(inst[c-1].optype!=2) or (!(detectRAW(inst[c-1],inst[c+2]))))
					{
						Instruction t=inst[c];
						inst.erase(inst.begin()+c);
						inst.insert(inst.begin()+p,t);
						done=true;
						//cout<<"c= "<<c<<" p="<<p<<endl;
						setdependances(inst,rangecanmovein);
						break;
					}
				}
			}
		}
	}

	for(int p=2; p<inst.size(); p++) {
		if(detectRAW(inst[p-1],inst[p]) and inst[p-1].optype==0) {

			done=false;
			int c=p-2;

			if ((c-1<0 or inst[c-1].optype!=0 or !detectRAW(inst[c-1],inst[p-2]))
			        and((c-2<0) or (inst[c-2].optype!=2) or (!detectRAW(inst[c-2],inst[p-2])))
			        and((inst[p-2].optype!=0) or ( !(detectRAW(inst[p-2],inst[p]))))
			        and((p-3<0)or(inst[p-3].optype!=2)or(!(detectRAW(inst[p-3],inst[p]))))) {
				while(c>=0 && rangecanmovein[p-1].first<c && !done) {
					if((c-1<0)
					        or(inst[c-1].optype!=2 or (!(detectRAW(inst[c-1],inst[c])))))

					{
						Instruction t=inst[p-1];
						inst.erase(inst.begin()+p-1);
						inst.insert(inst.begin()+c,t);
						done=true;
						//cout<<"c= "<<c<<" p="<<p<<endl;
						setdependances(inst,rangecanmovein);
						break;
					}
					c--;
				}
			}

			if(!done) {

				for(int i=p+1; i<inst.size(); i++)
				{	c=i;
					if((inst[p-1].optype!=0 or !detectRAW(inst[p-1],inst[c]))
					        and((inst[p-1].optype!=2) or (!detectRAW(inst[p-1],inst[p])))
					        and(rangecanmovein[i].first<=p-1 and !done)) {

						if((c+1==inst.size() or (inst[c-1].optype==1))
						        or ((inst[c-1].optype==0) && !(detectRAW(inst[c-1],inst[c+1])))
						        or( inst[c-2].optype==2 && !(detectRAW(inst[c-2],inst[c+1]))))
						{
							if((c+2>=inst.size()) or(inst[c-1].optype!=2) or (!(detectRAW(inst[c-1],inst[c+2]))))
							{
								Instruction t=inst[c];
								inst.erase(inst.begin()+c);
								inst.insert(inst.begin()+p,t);
								done=true;
								//cout<<"c= "<<c<<" p="<<p<<endl;
								setdependances(inst,rangecanmovein);
								break;
							}
						}


					}
				}
			}


		}


		if(detectRAW(inst[p-2],inst[p]) and inst[p-2].optype==2) {
			done=false;
			int c=p-3;
			if ((p-3<0 or (inst[p-2].optype==1) or ( (inst[p-3].optype==0 and !(detectRAW(inst[p-3],inst[p-1])))or(inst[p-3].optype==2 and !(detectRAW(inst[p-3],inst[p])))))
			        and((p-4<0)or(inst[p-4].optype!=2)or(!(detectRAW(inst[p-4],inst[p-1]))))) {
				while(c>=0 && rangecanmovein[p-2].first<c && !done) {
					if(((c-1)<0) or (inst[c-1].optype!=2) or (!(detectRAW(inst[c-1],inst[c]))))
					{
						Instruction t=inst[p-2];
						inst.erase(inst.begin()+p-2);
						inst.insert(inst.begin()+c,t);
						done=true;
						setdependances(inst,rangecanmovein);
						break;
					}
					c--;
				}
			}
			//c-3
			//c-2
			//c-1
			//p-1
			//c
			//...
			//p-2
			//p
			//c-2
			//c-1
			//c+1
			if(!done) {
				for(int i=p+1; i<inst.size(); i++)
				{	c=i;
					if((inst[p-1].optype!=0 or !detectRAW(inst[p-1],inst[p]))
					        and((inst[p-2].optype!=2) or (!detectRAW(inst[p-2],inst[c])))
					        and(rangecanmovein[i].first<p and !done)) {
						if((c+1==inst.size())
						        or ((inst[c-1].optype==0) && !(detectRAW(inst[c-1],inst[c+1])))
						        or( inst[c-2].optype==2 && !(detectRAW(inst[c-2],inst[c+1]))))

						{
							if((c+2>=inst.size()) or(inst[c-1].optype!=2) or ( !(detectRAW(inst[c-1],inst[c+2]))))
							{
								Instruction t=inst[c];
								inst.erase(inst.begin()+c);
								inst.insert(inst.begin()+p,t);
								done=true;
								//	cout<<"c= "<<c<<" p="<<p<<endl;
								setdependances(inst,rangecanmovein);
								break;
							}

						}
					}
				}
			}

		
		
			if(!done) {
				for(c=p-2; c>=0; c--){
					while(rangecanmovein[p-1].first<c and (c-1<0
				       or (inst[c-1].optype==1) or((inst[c-1].optype==0)&& !(detectRAW(inst[c-1],inst[p-1])))
			            or((inst[c-1].optype==2)&& !(detectRAW(inst[c-1],inst[c]))))
					and( c-2<0 or inst[c-2].optype!=2 or( inst[c-2].optype==2 && !(detectRAW(inst[c-2],inst[p-1]))))) {
					Instruction t=inst[p-1];
						inst.erase(inst.begin()+p-1);
						inst.insert(inst.begin()+c,t);
						done=true;
						setdependances(inst,rangecanmovein);
						break;

					}
			}}
				if(!done) {
				for(c=p-2; c<=0; c--){
					while(rangecanmovein[p-1].first<c and !done and (c-1<0
				       or ((inst[c-1].optype==1) or((inst[c-1].optype==0)&& !(detectRAW(inst[c-1],inst[p-1])))
			            or((inst[c-1].optype==2)&& !(detectRAW(inst[c-1],inst[c])))))
					and( c-2<0 or inst[c-2].optype!=2 or( inst[c-2].optype==2 && !(detectRAW(inst[c-2],inst[p-1]))))) {
					Instruction t=inst[p-1];
						inst.erase(inst.begin()+p-1);
						inst.insert(inst.begin()+c,t);
						done=true;
						setdependances(inst,rangecanmovein);
						break;

					}
			}}

		}

	}

}
string detectstalles(const Instruction& curr,const Instruction& prev,const Instruction& pprev,const Instruction& ppprev) {
	if(detectRAW(prev,curr))return "stall stall stall";
	else if(detectRAW(pprev,curr))return "stall stall";
	else if(detectRAW(ppprev,curr))return "stall";
	else return "yes";

}
void doReordering(vector<Instruction>&inst) {
	for(int i=0; i<inst.size(); i++) {
		Instruction &curr=inst[i];
		while(stallCycle[globalCycle]) {
			curr.timeline[globalCycle]="stall";
			globalCycle++;
		}
		curr.first_cycle=globalCycle;
		curr.timeline[globalCycle]="IF";
		globalCycle++;
		int tempcycle=globalCycle;
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		if(i==0) {
			curr.timeline[tempcycle]="ID";
			tempcycle++;
		}
		else if(i==1) {
			if(detectRAW(inst[0],inst[1])) {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
			else
			{
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
		}
		else if(i==2) {
			if(detectRAW(inst[1],inst[2])) {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
			else if(detectRAW(inst[0],inst[2])) {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
			else
			{
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}

		}
		else {
			if(detectstalles(inst[i],inst[i-1],inst[i-2],inst[i-3])=="stall stall stall") {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
			else if(detectstalles(inst[i],inst[i-1],inst[i-2],inst[i-3])=="stall stall") {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
			else if(detectstalles(inst[i],inst[i-1],inst[i-2],inst[i-3])=="stall") {
				curr.timeline[tempcycle]="stall";
				stallCycle[tempcycle]=true;
				tempcycle++;
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
			else
			{
				curr.timeline[tempcycle]="ID";
				tempcycle++;
			}
		}
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		curr.timeline[tempcycle]="EX";
		tempcycle++;
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		curr.timeline[tempcycle]="MA";
		tempcycle++;
		while(stallCycle[tempcycle]) {
			curr.timeline[tempcycle]="stall";
			tempcycle++;
		}
		curr.timeline[tempcycle]="WB";
		tempcycle++;
	}
}
bool test(const Instruction& a, const Instruction& b) {
	if (detectRAW(a, b) || detectWAR(a, b) || detectWAW(a, b))
		return false;
	return true;
}
bool canMove(size_t j, size_t insertPos, const vector<Instruction>& inst) {
	for (size_t k = insertPos; k < j; k++) {
		if (detectRAW(inst[k], inst[j]) || detectWAR(inst[k], inst[j]) || detectWAW(inst[k], inst[j]))
			return false;
	}
	return true;
}
void Reordering(vector<Instruction>&inst) {
	for (int i=0 ; i<inst.size() ; i++) {
		if(inst[i].optype == 1)
		{
			int j=i+1;
			while(j<inst.size() and (inst[j].optype !=0 and inst[i].s1==inst[j].des)) {
				if(inst[j].optype == 0 and inst[i].des==inst[j].s1)
				{
					for(int k=j+1 ; k<inst.size() ; k++ ) {
						if(inst[k].des == inst[j].des)
						{
							inst[k].des=inst[i].s1 ;
						}
						if(inst[k].s1 == inst[j].des)
						{
							inst[k].s1=inst[i].s1 ;
						}
						if(inst[k].s2 == inst[j].des)
						{
							inst[k].s2=inst[i].s1 ;
						}
					}
					inst.erase(inst.begin()+j);
					j--;
				}
				j++;
			}

		}
	}
	vector<Instruction> loads, indepInst, others ;
	for (auto &i : inst) {
		bool independent = true;
		for (auto &prev : inst) {
			if (&prev == &i) break;
			if (!test(prev, i)) {
				independent = false;
				break;
			}
		}
		if (independent)
			indepInst.push_back(i);
		else
			others.push_back(i);
	}

	for (size_t i = 0; i + 1 < others.size(); i++) {
		if (detectRAW(others[i], others[i+1]) || detectWAR(others[i], others[i+1]) || detectWAW(others[i], others[i+1])) {
			for (size_t j = i + 2; j < others.size(); j++) {
				if (canMove(j, i+1, others)) {
					Instruction temp = others[j];
					others.erase(others.begin() + j);
					others.insert(others.begin() + i + 1, temp);
					break;
				}
			}
		}
	}
	indepInst.insert(indepInst.end(), others.begin(), others.end());
	inst=indepInst;

}


void printPipelineTable(const vector<Instruction>& inst)
{
	int maxCycle = 0;
	for (const auto& ins : inst) {
		for (const auto& p : ins.timeline) {
			maxCycle = max(maxCycle, p.first);
		}
	}

	cout << "\nPipeline Execution Table:\n\n";
	cout << setw(6) << "Cyc | ";
	for (int i = 0; i < inst.size(); i++) {
		cout << setw(6) << ("I" + to_string(i));
	}
	cout << "\n";

	cout << string(6 + 3 + inst.size() * 6, '-') << "\n";

	for (int c = 1; c <= maxCycle; c++) {
		cout << setw(3) << c << " | ";

		for (int i = 0; i < inst.size(); i++) {
			auto it = inst[i].timeline.find(c);
			if (it != inst[i].timeline.end())
				cout << setw(6) << it->second;
			else
				cout << setw(6) << ".";
		}
		cout << "\n";
	}
	double speedup=(inst.size()*5.0)/maxCycle;
	cout<<"speedup= "<<speedup;

}


int main()
{
	vector<Instruction> inst;
	string line;
	int choic=0;
	cout<<"Enter 1 for Forwarding \nEnter 2 for Reordering With Forwarding\nEnter 3 for Reordering:\n";
	cin>>choic;
	getline(cin,line);
	cout<<"Enter instructions :\n\n";

	while(getline(cin,line)) {
		if(line.empty()) break;
		inst.emplace_back(line);
	}

	//---------print dependances ------------
	printDependances(inst);

	switch(choic) {
	case 1: {
		cout<<"\ninstructions are :\n";
		for(int i=0; i<inst.size(); i++)
		{
			cout<<inst[i].instr<<endl;
		}
		Forwarding(inst);
		printPipelineTable(inst);
		break;
	}
	case 2: {
		cout<<"\noriginal instructions are:\n";
		for(int i=0; i<inst.size(); i++)
		{
			cout<<inst[i].instr<<endl;
		}
		ReorderingWithForwarding(inst);
		cout<<"\nReordering instructions are\n";
		for(int i=0; i<inst.size(); i++)
		{
			cout<<inst[i].instr<<endl;
		}
		Forwarding(inst);
		printPipelineTable(inst);
		break;
	}
	case 3: {
		cout<<"\noriginal instructions are:\n";
		for(int i=0; i<inst.size(); i++)
		{
			cout<<inst[i].instr<<endl;
		}
		Reordering(inst);
		doReordering(inst);
		cout<<"\nReordering instructions are\n";
		for(int i=0; i<inst.size(); i++)
		{
			cout<<inst[i].instr<<endl;
		}
		printPipelineTable(inst);
	}
	}

	return 0;
}
