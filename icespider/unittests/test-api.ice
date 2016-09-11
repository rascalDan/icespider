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

	interface TestApi {
		SomeModel index();
		SomeModel withParams(string s, int i);
		void returnNothing(string s);
		void complexParam(optional(0) string s, SomeModel m);
	};
};

