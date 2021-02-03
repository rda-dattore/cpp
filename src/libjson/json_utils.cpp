#include <string>
#include <vector>

namespace JSONUtils {

void find_csv_ends(const std::string& json,std::vector<size_t>& csv_ends)
{
  auto in_enclosure=0,in_quotes=0;
  for (size_t n=0; n < json.length(); ++n) {
    switch (json[n]) {
	case '\\':
	{
	  ++n;
	  break;
	}
	case '{':
	case '[':
	{
	  if (in_quotes == 0) {
	    ++in_enclosure;
	  }
	  break;
	}
	case '}':
	case ']':
	{
	  if (in_quotes == 0) {
	    --in_enclosure;
	  }
	  break;
	}
	case '"':
	{
	  in_quotes=1-in_quotes;
	  break;
	}
	case ',':
	{
	  if (in_enclosure == 0 && in_quotes == 0) {
	    csv_ends.emplace_back(n);
	  }
	  break;
	}
	default:
	{}
    }
  }
  csv_ends.emplace_back(json.length());
}

} // end namespace JSONUtils
