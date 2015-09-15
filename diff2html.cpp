#include <boost/algorithm/string/replace.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>

using namespace std;

void makeOrgChg(const string& diffTxt, string& original, string& changed, string& originalHtml) {
	string::size_type findLastPos = diffTxt.find_first_not_of('\n', 0);
	string::size_type findPos = diffTxt.find_first_of('\n', findLastPos);
	static int lineCnt = 1;
	static char before;
	while (findPos != string::npos || findLastPos != string::npos) {
		string line = diffTxt.substr(findLastPos, findPos-findLastPos);
		printf("%d: %s\n", lineCnt, line.c_str());
		if (lineCnt > 3) {
			if (line.length() > 2 && line.substr(0,2) == "@@") {
				original += "\n";
				changed += "\n";
				originalHtml += "\n";
			} else {
				char diff = line[0];
				line = line.substr(1);
				boost::replace_all(line, "&", "&amp;");
				boost::replace_all(line, "<", "&lt;");
				boost::replace_all(line, ">", "&gt;");
				switch (diff) {
					case ' ' :
						original += line + "\n";
						changed += line + "\n";
						originalHtml += line + "\n";
						break;
					case '-' :
						original += line + "\n";
						originalHtml += line + "\n";
						break;
					case '+' :
						changed += line + "\n";
						if (before != '-') originalHtml += "\n";
						break;
				}
				before = diff;
			}
		}
		findLastPos = diffTxt.find_first_not_of('\n', findPos);
		findPos = diffTxt.find_first_of('\n', findLastPos);
		++lineCnt;
	}
}

int main(int argc, char* argv[])
{
	if (argc != 5) {
		printf("diff2html [file1] [file2] [template] [output]\n");
		exit(1);
	}

	string command = "/usr/bin/diff -u ";
	command += argv[1];
	command += " ";
	command += argv[2];

	FILE* fp = NULL;
	fp = popen(command.c_str(), "r");
	assert(fp != NULL);

	string txt = "";
	string original = "";
	string changed = "";
	string originalHtml = "";
	while (!feof(fp)) {
		unsigned char buff[65536] = {0,};
		fread(buff, sizeof(buff)-1, 1, fp);
		assert(ferror(fp) == 0);

		txt += (const char*)buff;

		string::size_type pos = txt.find_last_of('\n');
		if (pos == string::npos) continue;

		string diffTxt = txt.substr(0,pos);
		(void) makeOrgChg(diffTxt, original, changed, originalHtml);

		txt = txt.substr(pos+1);
	}

	(void) makeOrgChg(txt, original, changed, originalHtml);

	boost::replace_all(originalHtml, "\"", "&quot;");
	boost::replace_all(originalHtml, "\'", "&#39;");
	boost::replace_all(originalHtml, " ", "&nbsp;");
	boost::replace_all(originalHtml, "\n", "<br />");

	int status = pclose(fp);
	assert(status != -1);

	if (WIFEXITED(status)) {
		printf("exit:%d\n", WEXITSTATUS(status));
	}

	string file = argv[4];
	file += ".org";
	FILE* wfp = fopen(file.c_str(), "w");
	assert(wfp != NULL);
	fwrite(original.c_str(), original.length(), 1, wfp);
	fclose(wfp);

	file = argv[4];
	file += ".chg";
	wfp = fopen(file.c_str(), "w");
	assert(wfp != NULL);
	fwrite(changed.c_str(), changed.length(), 1, wfp);
	fclose(wfp);

	file = argv[4];
	file += ".orgHtml";
	wfp = fopen(file.c_str(), "w");
	assert(wfp != NULL);
	fwrite(originalHtml.c_str(), originalHtml.length(), 1, wfp);
	fclose(wfp);

	fp = fopen(argv[3], "r");
	assert(fp != NULL);

	string tpl = "";
	while (!feof(fp)) {
		unsigned char buff[65536] = {0,};
		fread(buff, sizeof(buff)-1, 1, fp);
		assert(ferror(fp) == 0);
		tpl += (const char*)buff;
	}
	boost::replace_all(tpl, "\r", "");
	boost::replace_all(tpl, "[original]", original);
	boost::replace_all(tpl, "[changed]", changed);
	boost::replace_all(tpl, "[originalHtml]", originalHtml);

	file = argv[4];
	file += ".html";
	wfp = fopen(file.c_str(), "w");
	assert(wfp != NULL);
	fwrite(tpl.c_str(), tpl.length(), 1, wfp);
	fclose(wfp);

	return 0;
}
