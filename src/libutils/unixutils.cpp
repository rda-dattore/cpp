#include <iomanip>
#include <fstream>
#include <string>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <strutils.hpp>
#include <utils.hpp>
#include <tempfile.hpp>

namespace unixutils {

const char *cat(std::string filename)
{
  static std::unique_ptr<char[]> buffer=nullptr;
  std::ifstream ifs(filename);
  if (!ifs.is_open()) {
    return nullptr;
  }
  ifs.seekg(0,std::ios::end);
  size_t file_size=ifs.tellg();
  buffer.reset(new char[file_size+1]);
  ifs.seekg(0,std::ios::beg);
  ifs.read(buffer.get(),file_size);
  ifs.close();
  buffer[file_size]='\0';
  return buffer.get();
}

std::string host_name()
{
  char host[256];
  gethostname(host,256);
  return host;
}

bool hosts_loaded(std::list<std::string>& hosts,std::string rdadata_home)
{
  std::ifstream ifs(rdadata_home+"/bin/conf/webhosts.conf");
  if (ifs.is_open()) {
    char line[256];
    ifs.getline(line,256);
    while (!ifs.eof()) {
	if (line[0] != '#') {
	  hosts.emplace_back(line);
	}
	ifs.getline(line,256);
    }
    ifs.close();
    return true;
  }
  else {
    return false;
  }
}

long long little_endian(unsigned char *buffer,size_t num_bytes)
{
  long long l=0;
  int nb=num_bytes-1;
  while (nb >= 0) {
    l=(l << 8) | static_cast<long long>(buffer[nb]);
    --nb;
  }
  return l;
}

std::string unix_args_string(int argc,char **argv,char separator)
{
  std::string unix_args;
  if (argc > 1) {
    unix_args=argv[1];
    char s[2];
    s[0]=separator;
    s[1]='\0';
    for (int n=2; n < argc; ++n) {
	unix_args+=(std::string(s)+argv[n]);
    }
  }
  return unix_args;
}

void dump(const unsigned char *buffer,int length,DumpType dump_type)
{
  if (dump_type == DumpType::bytes) {
    for (int n=0; n < length; n+=16) {
	for (int m=0; m < 16; ++m) {
	  if ( (n+m) >= length) {
	    break;
	  }
	  if (static_cast<int>(buffer[n+m]) >= 32 && static_cast<int>(buffer[n+m]) < 127) {
	    std::cout << "    " << static_cast<char>(buffer[n+m]);
	  }
	  else {
	    std::cout << std::setw(4) << std::setfill(' ') << static_cast<int>(buffer[n+m]) << "d";
	  }
	}
	std::cout << std::endl;
    }
  }
  else if (dump_type == DumpType::bits) {
    for (int n=0; n < length; ++n) {
	std::cout << ((buffer[n] & 0x80) >> 7) << ((buffer[n] & 0x40) >> 6) << ((buffer[n] & 0x20) >> 5) << ((buffer[n] & 0x10) >> 4) << ((buffer[n] & 0x8) >> 3) << ((buffer[n] & 0x4) >> 2) << ((buffer[n] & 0x2) >> 1) << (buffer[n] & 0x1);
    }
    std::cout << std::endl;
  }
}

void sendmail(std::string to_list,std::string from,std::string bcc_list,std::string subject,std::string body)
{
  std::string mail_command="echo \"To: "+to_list+"\nFrom: "+from+"\n";
  if (bcc_list.length() > 0) {
    mail_command+="Bcc: "+bcc_list+"\n";
  }
  strutils::replace_all(body,"\"","\\\"");
  strutils::replace_all(body,"`","\\`");
  mail_command+="Subject: "+subject+"\n\n"+body+"\" |/usr/sbin/sendmail -t";
  if (system(mail_command.c_str())) { } // suppress compiler warning
}

void untar(std::string dirname,std::string filename)
{
  auto tfile=TempFile(dirname,"").name();
  auto ifp=fopen64(filename.c_str(),"r");
  if (ifp == nullptr) {
    std::cerr << "Error opening " << filename << std::endl;
    exit(1);
  }
  auto ofp=fopen64(tfile.c_str(),"w");
  if (ofp == nullptr) {
    std::cerr << "Error opening tar output file" << std::endl;
    exit(1);
  }
  unsigned char buffer[512];
  char *cbuf=reinterpret_cast<char *>(buffer);
  long long size=0,num_out=-1;
  while (fread(buffer,1,512,ifp) > 0) {
    if (num_out == -1) {
	auto m=0;
	for (size_t n=148; n < 156; ++n) {
	  if ((buffer[n] >= '0' && buffer[n] <= '9') || buffer[n] == ' ') {
	    ++m;
	  }
	  else {
	    n=156;
	  }
	}
	int cksum;
	strutils::strget(&cbuf[148],cksum,m);
	auto sum=256;
	for (size_t n=0; n < 148; ++n) {
	  sum+=static_cast<int>(buffer[n]);
	}
	for (size_t n=156; n < 512; ++n) {
	  sum+=static_cast<int>(buffer[n]);
	}
	if (sum != xtox::otoi(cksum)) {
	  break;
	}
	strutils::strget(&cbuf[124],size,11);
	size=xtox::otoll(size);
	if (size > 0)
	  num_out=0;
    }
    else {
	if ( (num_out+512) < size) {
	  fwrite(buffer,1,512,ofp);
	  num_out+=512;
	}
	else {
	  fwrite(buffer,1,size-num_out,ofp);
	  num_out=-1;
	}
    }
  }
  fclose(ifp);
  fclose(ofp);
  if (system(("mv "+tfile+" "+filename).c_str()) != 0) {
    std::cerr << "Error creating tar file" << std::endl;
    exit(1);
  }
}

bool tar(std::string tarfile,std::list<std::string>& filenames)
{
  char empty[512];
  char null_char='\0';
  for (size_t n=0; n < 512; ++n) {
    empty[n]=null_char;
  }
  auto ofp=fopen64(tarfile.c_str(),"w");
  size_t num_out=0;
  for (const auto& file : filenames) {
    auto name=file;
    struct stat buf;
    if (stat(name.c_str(),&buf) != 0) {
	if (!strutils::has_beginning(name,"/") && strutils::contains(name,"/")) {
	  if (stat(("/"+name).c_str(),&buf) != 0) {
	    fclose(ofp);
	    return false;
	  }
	}
	else {
	  fclose(ofp);
	  return false;
	}
    }
    auto link_indicator='0';
    if (buf.st_mode > 32768) {
	buf.st_mode-=32768;
    }
    if (buf.st_mode >= 16384) {
	if (!strutils::has_ending(name,"/")) {
	  name+="/";
	}
	buf.st_size=0;
	buf.st_mode-=16384;
	link_indicator='5';
    }
// write the file header
// chars 0-99 are filename
    char header[512];
    size_t n=0;
    for (; n < name.length(); ++n) {
	header[n]=name[n];
    }
    while (n < 100) {
	header[n++]='\0';
    }
// chars 100-107 are file mode
    auto mode=strutils::ftos(xtox::itoo(buf.st_mode),7,0,'0');
    for (; n < 107; ++n) {
	header[n]=mode[n-100];
    }
    header[n++]='\0';
// chars 108-115 are owner user id
    auto user_id=strutils::ftos(xtox::itoo(buf.st_uid),7,0,'0');
    for (; n < 115; ++n) {
	header[n]=user_id[n-108];
    }
    header[n++]='\0';
// chars 116-123 are owner group id
    auto group_id=strutils::ftos(xtox::itoo(buf.st_gid),7,0,'0');
    for (; n < 123; ++n) {
	header[n]=group_id[n-116];
    }
    header[n++]='\0';
// chars 124-135 are file size in bytes
    auto file_size=strutils::lltos(xtox::itoo(buf.st_size),11,'0');
    for (; n < 135; ++n) {
	header[n]=file_size[n-124];
    }
    header[n++]='\0';
// chars 136-147 are last modification time
    auto mod_time=strutils::lltos(xtox::ltoo(buf.st_mtime),11,'0');
    for (; n < 147; ++n) {
	header[n]=mod_time[n-136];
    }
    header[n++]='\0';
// chars 148-155 are the header checksum - clear them for now
    for (; n < 155; ++n) {
	header[n]='\0';
    }
    header[n++]='\0';
// char 156 is the link indicator
    header[n++]=link_indicator;
// chars 157-256 are name of linked file
    for (; n < 256; ++n) {
	header[n]='\0';
    }
    header[n++]='\0';
// chars 257-262 are 'ustar\0'
    header[n++]='u';
    header[n++]='s';
    header[n++]='t';
    header[n++]='a';
    header[n++]='r';
    header[n++]='\0';
// chars 263-264 are ustar version
    header[n++]='0';
    header[n++]='0';
// chars 265-296 are owner user name
    auto pwd=getpwuid(buf.st_uid);
    std::string pw_name;
    if (pwd != nullptr) {
	pw_name=pwd->pw_name;
    }
    else {
	pw_name="";
    }
    for (; n < 265+pw_name.length(); ++n) {
	header[n]=pw_name[n-265];
    }
    while (n < 296) {
	header[n++]='\0';
    }
    header[n++]='\0';
// chars 297-328 are owner group name
    auto grp=getgrgid(buf.st_gid);
    std::string gr_name;
    if (grp != nullptr) {
	gr_name=grp->gr_name;
    }
    else {
	gr_name="";
    }
    for (; n < 297+gr_name.length(); ++n) {
	header[n]=gr_name[n-297];
    }
    while (n < 328) {
	header[n++]='\0';
    }
    header[n++]='\0';
// chars 329-336 are the device major number
    auto dev_major=strutils::ftos(xtox::itoo(major(buf.st_dev)),7,0,'0');
    for (; n < 336; ++n) {
	header[n]=dev_major[n-329];
    }
    header[n++]='\0';
// chars 337-344 are the device minor number
    auto dev_minor=strutils::ftos(xtox::itoo(minor(buf.st_dev)),7,0,'0');
    for (; n < 344; ++n) {
	header[n]=dev_minor[n-337];
    }
    header[n++]='\0';
// chars 345-499 are the filename prefix
    for (; n < 500; ++n) {
	header[n]='\0';
    }
// rest of 512-byte header
    for (; n < 512; ++n) {
	header[n]='\0';
    }
// compute checksum
    auto cksum=256;
    for (n=0; n < 512; ++n) {
	cksum+=static_cast<int>(header[n]);
    }
    auto s_cksum=strutils::ftos(xtox::itoo(cksum),7,0,'0');
    for (n=148; n < 155; ++n) {
	header[n]=s_cksum[n-148];
    }
    header[n++]='\0';
    fwrite(header,1,512,ofp);
    num_out+=512;
    if (buf.st_size > 0) {
	std::ifstream ifs(name.c_str());
	if (!ifs.is_open()) {
	  ifs.open(("/"+name).c_str(),std::ifstream::in);
	  if (!ifs.is_open()) {
	    std::cerr << "Error opening file " << name << std::endl;
	    return false;
	  }
	}
	while (!ifs.eof()) {
	  char buffer[32768];
	  ifs.read(buffer,32768);
	  auto num=ifs.gcount();
	  fwrite(buffer,1,num,ofp);
	  num_out+=num;
	}
	ifs.close();
	ifs.clear();
	while ( (num_out % 512) != 0) {
	  fwrite(&null_char,1,1,ofp);
	  ++num_out;
	}
    }
    std::cout << "a " << file << ", " << buf.st_size << " bytes" << std::endl;
  }
// end the tar file with two zero-filled 512-byte blocks
  fwrite(empty,1,512,ofp);
  num_out+=512;
  fwrite(empty,1,512,ofp);
  num_out+=512;
  while ( (num_out % 10240) != 0) {
    fwrite(empty,1,512,ofp);
    num_out+=512;
  }
  fclose(ofp);
  return true;
}

} // end namespace unixutils
