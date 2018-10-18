#include <string>
#include <vector>

namespace JSONUtils {

void find_csv_ends(const std::string& json,std::vector<size_t>& csv_ends)
{
  auto in_enclosure=0;
  for (size_t n=0; n < json.length(); ++n) {
    switch (json[n]) {
	case '{':
	case '[':
	{
	  ++in_enclosure;
	  break;
	}
	case '}':
	case ']':
	{
	  --in_enclosure;
	  break;
	}
	case ',':
	{
	  if (in_enclosure == 0) {
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
