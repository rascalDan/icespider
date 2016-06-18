module TestIceSpider {
	class SomeModel {
		string value;
	};

	interface TestApi {
		SomeModel index();	
		SomeModel withParams(string s, int i);
		void returnNothing(string s);
		void complexParam(string s, SomeModel m);
	};
};

