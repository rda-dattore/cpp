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

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::list;
using std::string;
using std::unique_ptr;
using strutils::ftos;
using strutils::lltos;
using strutils::replace_all;
using strutils::strget;
using xtox::itoo;

namespace unixutils {

const char *cat(string filename) {
  static unique_ptr<char[]> buffer(nullptr);
  ifstream ifs(filename);
  if (!ifs.is_open()) {
    return nullptr;
  }
  ifs.seekg(0, std::ios::end);
  size_t s = ifs.tellg();
  buffer.reset(new char[s + 1]);
  ifs.seekg(0, std::ios::beg);
  ifs.read(buffer.get(), s);
  ifs.close();
  buffer[s] = '\0';
  return buffer.get();
}

string host_name() {
  char h[256];
  gethostname(h, 256);
  return h;
}

bool hosts_loaded(list<string>& hosts, string rdadata_home) {
  ifstream ifs(rdadata_home + "/bin/conf/webhosts.conf");
  if (ifs.is_open()) {
    char l[256];
    ifs.getline(l, 256);
    while (!ifs.eof()) {
      if (l[0] != '#') {
        hosts.emplace_back(l);
      }
      ifs.getline(l, 256);
    }
    ifs.close();
    return true;
  }
  return false;
}

long long little_endian(unsigned char *buffer, size_t num_bytes) {
  long long l = 0; // return value
  int nb = num_bytes - 1;
  while (nb >= 0) {
    l = l << 8 | static_cast<long long>(buffer[nb]);
    --nb;
  }
  return l;
}

string unix_args_string(int argc, char **argv, char separator) {
  string s; // return value
  if (argc > 1) {
    s = argv[1];
    for (int n = 2; n < argc; ++n) {
      s.append(1, separator);
      s += argv[n];
    }
  }
  return s;
}

void dump(const unsigned char *buffer, int length, DumpType dump_type) {
  if (dump_type == DumpType::bytes) {
    for (int n = 0; n < length; n += 16) {
      for (int m = 0; m < 16; ++m) {
        if (n + m >= length) {
          break;
        }
        if (static_cast<int>(buffer[n + m]) >= 32 && static_cast<int>(buffer[n +
            m]) < 127) {
          cout << "    " << static_cast<char>(buffer[n + m]);
        } else {
          cout << std::setw(4) << std::setfill(' ') << static_cast<int>(buffer[n
              + m]) << "d";
        }
      }
      cout << endl;
    }
  } else if (dump_type == DumpType::bits) {
    for (int n = 0; n < length; ++n) {
      cout << (buffer[n] & 0x80 >> 7) << (buffer[n] & 0x40 >> 6) << (buffer[n] &
          0x20 >> 5) << (buffer[n] & 0x10 >> 4) << (buffer[n] & 0x8 >> 3) <<
          (buffer[n] & 0x4 >> 2) << (buffer[n] & 0x2 >> 1) << (buffer[n] & 0x1);
    }
    cout << endl;
  }
}

void sendmail(string to_list, string from, string bcc_list, string subject,
    string body) {
  string m = "echo \"To: " + to_list + "\nFrom: " + from + "\n";
  if (!bcc_list.empty()) {
    m += "Bcc: " + bcc_list + "\n";
  }
  replace_all(body, "\"", "\\\"");
  replace_all(body, "`", "\\`");
  m += "Subject: " + subject + "\n\n" + body + "\" |/usr/sbin/sendmail -t";
  if (system(m.c_str())) { } // suppress compiler warning
}

void untar(string dirname, string filename) {
  auto tf = TempFile(dirname, "").name();
  auto fp_i = fopen64(filename.c_str(), "r");
  if (fp_i == nullptr) {
    cerr << "Error opening " << filename << endl;
    exit(1);
  }
  auto fp_o = fopen64(tf.c_str(), "w");
  if (fp_o == nullptr) {
    cerr << "Error opening tar output file" << endl;
    exit(1);
  }
  unsigned char buffer[512];
  char *b = reinterpret_cast<char *>(buffer);
  long long sz = 0, n = -1;
  while (fread(buffer, 1, 512, fp_i) > 0) {
    if (n == -1) {
      auto m = 0;
      for (size_t n = 148; n < 156; ++n) {
        if ((buffer[n] >= '0' && buffer[n] <= '9') || buffer[n] == ' ') {
          ++m;
        } else {
          n = 156;
        }
      }
      int c;
      strget(&b[148], c, m);
      auto s = 256;
      for (size_t n = 0; n < 148; ++n) {
        s += static_cast<int>(buffer[n]);
      }
      for (size_t n = 156; n < 512; ++n) {
        s += static_cast<int>(buffer[n]);
      }
      if (s != xtox::otoi(c)) {
        break;
      }
      strget(&b[124], sz, 11);
      sz = xtox::otoll(sz);
      if (sz > 0)
        n = 0;
    } else {
      if (n + 512 < sz) {
        fwrite(buffer, 1, 512, fp_o);
        n += 512;
      } else {
        fwrite(buffer, 1, sz - n, fp_o);
        n =- 1;
      }
    }
  }
  fclose(fp_i);
  fclose(fp_o);
  if (system(("mv " + tf + " " + filename).c_str()) != 0) {
    cerr << "Error creating tar file" << endl;
    exit(1);
  }
}

bool tar(string tarfile, list<string>& filenames) {
  char nc = '\0';
  auto fp_o = fopen64(tarfile.c_str(), "w");
  size_t byts = 0;
  for (const auto& f : filenames) {
    auto fn = f;
    struct stat buf;
    if (stat(fn.c_str(), &buf) != 0) {
      if (fn[0] != '/' && strutils::contains(fn, "/")) {
        if (stat(("/" + fn).c_str(), &buf) != 0) {
          fclose(fp_o);
          return false;
        }
      } else {
        fclose(fp_o);
        return false;
      }
    }
    auto li = '0'; // link indicator
    if (buf.st_mode > 32768) {
      buf.st_mode -= 32768;
    }
    if (buf.st_mode >= 16384) {
      if (fn.back() != '/') {
        fn += "/";
      }
      buf.st_size = 0;
      buf.st_mode -= 16384;
      li = '5';
    }

    // write the file header
    // chars 0-99 are filename
    char hd[512];
    size_t n = 0;
    for (; n < fn.length(); ++n) {
      hd[n] = fn[n];
    }
    while (n < 100) {
      hd[n++] = '\0';
    }

    // chars 100-107 are file mode
    auto x = ftos(itoo(buf.st_mode), 7, 0, '0');
    for (; n < 107; ++n) {
      hd[n] = x[n - 100];
    }
    hd[n++] = '\0';

    // chars 108-115 are owner user id
    x = ftos(itoo(buf.st_uid), 7, 0, '0');
    for (; n < 115; ++n) {
      hd[n] = x[n - 108];
    }
    hd[n++] = '\0';

    // chars 116-123 are owner group id
    x = ftos(itoo(buf.st_gid), 7, 0, '0');
    for (; n < 123; ++n) {
      hd[n] = x[n - 116];
    }
    hd[n++] = '\0';

    // chars 124-135 are file size in bytes
    auto l = lltos(itoo(buf.st_size), 11, '0');
    for (; n < 135; ++n) {
      hd[n] = l[n - 124];
    }
    hd[n++] = '\0';

    // chars 136-147 are last modification time
    l = lltos(xtox::ltoo(buf.st_mtime), 11, '0');
    for (; n < 147; ++n) {
      hd[n] = l[n - 136];
    }
    hd[n++] = '\0';

    // chars 148-155 are the hd checksum - clear them for now
    for (; n < 155; ++n) {
      hd[n] = '\0';
    }
    hd[n++] = '\0';

    // char 156 is the link indicator
    hd[n++] = li;

    // chars 157-256 are name of linked file
    for (; n < 256; ++n) {
      hd[n] = '\0';
    }
    hd[n++] = '\0';

    // chars 257-262 are 'ustar\0'
    hd[n++] = 'u';
    hd[n++] = 's';
    hd[n++] = 't';
    hd[n++] = 'a';
    hd[n++] = 'r';
    hd[n++] = '\0';

    // chars 263-264 are ustar version
    hd[n++] = '0';
    hd[n++] = '0';

    // chars 265-296 are owner user name
    auto p = getpwuid(buf.st_uid);
    string pw;
    if (p != nullptr) {
      pw = p->pw_name;
    } else {
      pw = "";
    }
    for (; n < 265 + pw.length(); ++n) {
      hd[n] = pw[n - 265];
    }
    while (n < 296) {
      hd[n++] = '\0';
    }
    hd[n++] = '\0';

    // chars 297-328 are owner group name
    auto gid = getgrgid(buf.st_gid);
    string g;
    if (gid != nullptr) {
      g = gid->gr_name;
    } else {
      g = "";
    }
    for (; n < 297 + g.length(); ++n) {
      hd[n] = g[n - 297];
    }
    while (n < 328) {
      hd[n++] = '\0';
    }
    hd[n++] = '\0';

    // chars 329-336 are the device major number
//    x = ftos(itoo(major(buf.st_dev)), 7, 0, '0');
x = ftos(0, 7, 0, '0');
    for (; n < 336; ++n) {
      hd[n] = x[n - 329];
    }
    hd[n++] = '\0';

    // chars 337-344 are the device minor number
//    x = ftos(itoo(minor(buf.st_dev)), 7, 0, '0');
x = ftos(0, 7, 0, '0');
    for (; n < 344; ++n) {
      hd[n] = x[n - 337];
    }
    hd[n++] = '\0';

    // chars 345-499 are the filename prefix
    for (; n < 500; ++n) {
      hd[n] = '\0';
    }

    // rest of 512-byte header
    for (; n < 512; ++n) {
      hd[n] = '\0';
    }

    // compute checksum
    auto s = 256;
    for (n = 0; n < 512; ++n) {
      s += static_cast<int>(hd[n]);
    }
    x = ftos(itoo(s), 7, 0, '0');
    for (n = 148; n < 155; ++n) {
      hd[n] = x[n - 148];
    }
    hd[n++] = '\0';
    fwrite(hd, 1, 512, fp_o);
    byts += 512;
    if (buf.st_size > 0) {
      ifstream ifs(fn.c_str());
      if (!ifs.is_open()) {
        ifs.open(("/" + fn).c_str(), ifstream::in);
        if (!ifs.is_open()) {
          cerr << "Error opening file " << fn << endl;
          return false;
        }
      }
      while (!ifs.eof()) {
        char b[32768];
        ifs.read(b, 32768);
        auto n = ifs.gcount();
        fwrite(b, 1, n, fp_o);
        byts += n;
      }
      ifs.close();
      ifs.clear();
      while (byts % 512 != 0) {
        fwrite(&nc, 1, 1, fp_o);
        ++byts;
      }
    }
    cout << "a " << f << ", " << buf.st_size << " bytes" << endl;
  }

  // end the tar file with two zero-filled 512-byte blocks
  char z[512];
  for (size_t n = 0; n < 512; ++n) {
    z[n] = nc;
  }
  fwrite(z, 1, 512, fp_o);
  byts += 512;
  fwrite(z, 1, 512, fp_o);
  byts += 512;
  while (byts % 10240 != 0) {
    fwrite(z, 1, 512, fp_o);
    byts += 512;
  }
  fclose(fp_o);
  return true;
}

} // end namespace unixutils
