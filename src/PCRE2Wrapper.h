#ifndef PCRE2Wrapper_H
#define PCRE2Wrapper_H

#include <climits> // CHAR_BIT

#include <nan.h>
#include <jpcre2.hpp>

typedef jpcre2::select<char> jp;

class PCRE2Wrapper : public Nan::ObjectWrap {
public:
	static void Init(std::string name, v8::Local<v8::Object> exports);

private:
	static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
	
	static void Study(const Nan::FunctionCallbackInfo<v8::Value>& info);
	
	static void Test(const Nan::FunctionCallbackInfo<v8::Value>& info);
	static void Exec(const Nan::FunctionCallbackInfo<v8::Value>& info);
	static void Match(const Nan::FunctionCallbackInfo<v8::Value>& info);
	static void Replace(const Nan::FunctionCallbackInfo<v8::Value>& info);
	
	static void ToString(const Nan::FunctionCallbackInfo<v8::Value>& info);	
	static void HasModifier(const Nan::FunctionCallbackInfo<v8::Value>& info);
	
	static void PropertyGetter(v8::Local<v8::String> property, const Nan::PropertyCallbackInfo<v8::Value>& info);
	static NAN_SETTER(PropertySetter);
	
	static Nan::Persistent<v8::Function> constructor;
	
	// -- non static goes here
	
	// -- methods

	explicit PCRE2Wrapper();
	~PCRE2Wrapper();
	
	// -- fields
	
	std::string flags; //tymaczasowe do momentu aż dowiemy się dlaczego getModifiers() zwraca zawsze pusty string
	
	jp::Regex re;
	jp::RegexMatch rm;
	jp::RegexReplace rr;
	jp::MatchEvaluator me;
	
	jpcre2::VecOff vec_eoff;
	
	jp::VecNum vec_num;
	jp::VecNas vec_nas;
	jp::VecNtN vec_ntn;
	
	bool global;
	size_t lastIndex;
};

#endif