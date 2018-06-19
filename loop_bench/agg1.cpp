#include <string>
#include <cstdlib>
#include <vector>
using namespace std;

void serial(int nlines, string *A) {
	vector<int> res;
        

	int pushed = 0;
	int count = 0;

	for(int i=0;i<nlines;i++) {
		string type = A[2*i];
		string id = A[2*i+1];
		if(atoi(id.c_str()) == 1579990) {
			if(type=="PushEvent") {
				pushed = 1;
				count = 0;
			} else if(type=="PullRequestEvent" && pushed == 1) {
				pushed = 0;
				res.push_back(count);
			} else if(pushed) {
				count++;
			}
		}
	}

}
