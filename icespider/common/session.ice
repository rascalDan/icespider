#ifndef ICESPIDER_SESSION_ICE
#define ICESPIDER_SESSION_ICE

module IceSpider {
	dictionary<string, string> Variables;

	exception SessionError {
		string what;
	};

	class Session {
		string id;
		long lastUsed;
		short duration;
		Variables variables;
	};

	interface SessionManager {
		Session createSession() throws SessionError;
		Session getSession(string id) throws SessionError;
		void updateSession(Session session) throws SessionError;
		void destroySession(string id) throws SessionError;
	};
};

#endif

