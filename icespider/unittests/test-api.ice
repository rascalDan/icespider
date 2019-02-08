#define stringview ["cpp:view-type:std::string_view"] string
[["cpp:include:string_view_support.h"]]
module TestIceSpider {
	class SomeModel {
		string value;
	};

	struct Mash1 {
		SomeModel a;
		SomeModel b;
	};

	class Mash2 {
		SomeModel a;
		SomeModel b;
		string s;
	};

	["cpp:ice_print"]
	exception Ex {
		string message;
	};

	interface TestApi {
		int simple() throws Ex;
		string simplei(int i);
		SomeModel index();
		SomeModel withParams(string s, int i) throws Ex;
		void returnNothing(stringview s) throws Ex;
		void complexParam(optional(0) string s, SomeModel m);
	};
	interface DummyPlugin {
	};
};

