#include <fstream>
#include <string>
#include <regex>
#include <sys/stat.h>
#include <hereDoc.hpp>
#include <strutils.hpp>

namespace hereDoc {

void print(std::string filename,Tokens* tokens,std::ostream& ofs,size_t indent_length)
{
  struct stat buf;
  if (stat(filename.c_str(),&buf) == 0) {
    std::string indent(indent_length,' ');
    std::string sb;
    sb.reserve(buf.st_size*2);
    std::ifstream ifs;
    ifs.open(filename.c_str());
    char line[32768];
    ifs.getline(line,32768);
    auto in_if=false;
    auto write_if=false;
    while (!ifs.eof()) {
	std::string sline=line;
	if (std::regex_search(sline,std::regex("^#IF"))) {
	  in_if=true;
	  auto sp=strutils::split(sline);
	  IfEntry ife;
	  if (tokens != NULL && tokens->ifs.found(sp[1],ife)) {
	    write_if=true;
	  }
	}
	else if (std::regex_search(sline,std::regex("^#ELSE"))) {
	  if (in_if) {
	    write_if=!write_if;
	  }
	}
	else if (std::regex_search(sline,std::regex("^#ENDIF"))) {
	  in_if=false;
	  write_if=false;
	}
	else if (!in_if || write_if) {
	  if (std::regex_search(sline,std::regex("^#REPEAT"))) {
	    auto sp=strutils::split(sline);
	    RepeatEntry re;
	    if (tokens != nullptr && tokens->repeats.found(sp[1],re)) {
		std::string repeat_block;
		ifs.getline(line,32768);
		while (!ifs.eof()) {
		  sline=line;
		  if (std::regex_search(sline,std::regex("^#ENDREPEAT"))) {
		    break;
		  }
		  else {
		    repeat_block+=(indent+sline+"\n");
		    ifs.getline(line,32768);
		  }
		}
		if (repeat_block.length() > 0) {
		  for (auto& item : *re.list) {
		    auto iparts=strutils::split(item,"<!>");
		    if (iparts.size() > 1) {
			auto rblock=repeat_block;
			for (auto& ipart : iparts) {
			  auto elems=strutils::split(ipart,"[!]");
			  if (elems.size() == 2) {
			    strutils::replace_all(rblock,re.key+"."+elems[0],elems[1]);
			  }
			}
			sb+=rblock;
		    }
		    else {
			sb+=(strutils::substitute(repeat_block,re.key,item));
		    }
		  }
		}
	    }
	    else {
		sb+=(indent+sline+"\n");
	    }
	  }
	  else {
	    sb+=(indent+sline+"\n");
	  }
	}
	ifs.getline(line,32768);
    }
    ifs.close();
    if (tokens != NULL) {
	for (auto& token : tokens->replaces) {
	  auto sp=strutils::split(token,"<!>");
	  strutils::replace_all(sb,sp[0],sp[1]);
	}
    }
    ofs << sb << std::endl;
  }
}

void print(std::string filename,Tokens* tokens)
{
  print(filename,tokens,std::cout,0);
}

std::string processRepeat(std::ifstream&,std::string,Tokens*,const std::string&);

void processIf(std::ifstream& ifs,std::string if_token,Tokens* tokens,std::string& sb,const std::string& indent,bool parent_print)
{
  strutils::trim(if_token);
  IfEntry ife;
  auto print= (tokens != nullptr && parent_print && (if_token == "_FOUND_" || tokens->ifs.found(if_token,ife))) ? true : false;
  char line[32768];
  ifs.getline(line,32768);
  while (!ifs.eof()) {
    std::string sline=line;
    if (std::regex_search(sline,std::regex("^#IF"))) {
	processIf(ifs,sline.substr(3),tokens,sb,indent,print);
    }
    else if (std::regex_search(sline,std::regex("^#ENDIF *"+if_token))) {
	break;
    }
    else if (std::regex_search(sline,std::regex("^#ELSE *"+if_token))) {
	print=!print;
    }
    else if (print) {
	if (std::regex_search(sline,std::regex("^#REPEAT"))) {
	  sb+=processRepeat(ifs,sline.substr(7),tokens,indent);
	}
	else {
	  sb+=(indent+sline+"\n");
	}
    }
    ifs.getline(line,32768);
  }
}

std::string processRepeat(std::ifstream& ifs,std::string repeat_token,Tokens* tokens,const std::string& indent)
{
  strutils::trim(repeat_token);
  std::string repeat_result,repeat_block;
  RepeatEntry re;
  auto print= (tokens != nullptr && tokens->repeats.found(repeat_token,re) && re.list->size() > 0) ? true : false;
  char line[32768];
  ifs.getline(line,32768);
  while (!ifs.eof()) {
    std::string sline=line;
    if (std::regex_search(sline,std::regex("^#REPEAT"))) {
	repeat_block+=processRepeat(ifs,sline.substr(7),tokens,indent);
    }
    else if (std::regex_search(sline,std::regex("^#ENDREPEAT *"+repeat_token))) {
	break;
    }
    else if (print) {
	repeat_block+=(indent+sline+"\n");
    }
    ifs.getline(line,32768);
  }
  if (!repeat_block.empty()) {
    for (auto& item : *re.list) {
	auto iparts=strutils::split(item,"<!>");
	if (iparts.size() > 1) {
	  auto r=repeat_block;
	  size_t sidx;
	  while ( (sidx=r.find("#IF")) != std::string::npos) {
	    auto if_token=r.substr(sidx+3,r.find("\n",sidx+3)-sidx-3);
	    strutils::trim(if_token);
	    if (std::regex_search(item,std::regex(if_token+"\\[!\\]"))) {
		r=r.substr(0,sidx)+r.substr(r.find("\n",sidx)+1);
		if ( (sidx=r.rfind("#ENDIF")) != std::string::npos) {
		  r=r.substr(0,sidx)+r.substr(r.find("\n",sidx)+1);
		}
	    }
	    else {
		auto eidx=r.rfind(if_token)+if_token.length();
		while (eidx < r.length() && r[eidx] != '\n') {
		  ++eidx;
		}
		r=r.substr(0,sidx)+r.substr(eidx+1);
	    }
	  }
	  for (auto& ipart : iparts) {
	    auto elems=strutils::split(ipart,"[!]");
	    if (elems.size() == 2) {
		strutils::replace_all(r,repeat_token+"."+elems[0],elems[1]);
	    }
	  }
	  repeat_result+=r;
	}
	else {
	  repeat_result+=strutils::substitute(repeat_block,repeat_token,item);
	}
    }
  }
  return repeat_result;
}

void print2(std::string filename,Tokens* tokens,std::ostream& ofs,size_t indent_length)
{
  struct stat buf;
  if (stat(filename.c_str(),&buf) == 0) {
    std::string indent(indent_length,' ');
    std::string sb;
    sb.reserve(buf.st_size*2);
    std::ifstream ifs;
    ifs.open(filename.c_str());
    char line[32768];
    ifs.getline(line,32768);
    while (!ifs.eof()) {
	std::string sline=line;
	if (std::regex_search(sline,std::regex("^#IF"))) {
	  processIf(ifs,sline.substr(3),tokens,sb,indent,true);
	}
	else if (std::regex_search(sline,std::regex("^#REPEAT"))) {
	  sb+=processRepeat(ifs,sline.substr(7),tokens,indent);
	}
	else {
	  sb+=(indent+sline+"\n");
	}
	ifs.getline(line,32768);
    }
    ifs.close();
    if (tokens != nullptr) {
	for (auto& replace : tokens->replaces) {
	  auto rparts=strutils::split(replace,"<!>");
	  strutils::replace_all(sb,rparts[0],rparts[1]);
	}
    }
    ofs << sb.substr(0,sb.length()-1) << std::endl;
  }
}

} // end namespace hereDoc
