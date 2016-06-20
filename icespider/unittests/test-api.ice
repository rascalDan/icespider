module TestIceSpider {
	class SomeModel {
		string value;
	};

	interface TestApi {
		SomeModel index();	
		SomeModel withParams(string s, int i);
		void returnNothing(string s);
		void complexParam(optional(0) string s, SomeModel m);
	};
};

