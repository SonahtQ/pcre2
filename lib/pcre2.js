var lib = require('../build/Release/pcre2');

module.exports = {
	PCRE2: lib.PCRE2,
	PCRE2JIT: lib.PCRE2JIT
};

if (Symbol) {
	if (Symbol.match) {
		lib.PCRE2.prototype[Symbol.match] = lib.PCRE2JIT.prototype[Symbol.match] = function(str) {
			return this.match(str);
		}
	}
	
	if (Symbol.replace) {
		lib.PCRE2.prototype[Symbol.replace] = lib.PCRE2JIT.prototype[Symbol.replace] = function(str, repl) {
			return this.replace(str, repl);
		}
	}
	
	/*
	if (Symbol.search) {
		lib.PCRE2.prototype[Symbol.search] = lib.PCRE2JIT.prototype[Symbol.search] = function(str) {
			return this.search(str);
		}
	}
	*/
}