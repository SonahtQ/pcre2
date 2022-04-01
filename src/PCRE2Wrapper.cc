#include "PCRE2Wrapper.h"

Nan::Persistent<v8::Function> PCRE2Wrapper::constructor;

PCRE2Wrapper::PCRE2Wrapper() {
	lastIndex = 0;
	global = false;
}

PCRE2Wrapper::~PCRE2Wrapper() {
}

void PCRE2Wrapper::Init(std::string name, v8::Local<v8::Object> exports) {
	Nan::HandleScope scope;

	v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
	tpl->SetClassName(Nan::New(name).ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	
	Nan::SetPrototypeMethod(tpl, "test", PCRE2Wrapper::Test);
	Nan::SetPrototypeMethod(tpl, "exec", PCRE2Wrapper::Exec);
	Nan::SetPrototypeMethod(tpl, "match", PCRE2Wrapper::Match);
	Nan::SetPrototypeMethod(tpl, "replace", PCRE2Wrapper::Replace);
	
	Nan::SetPrototypeMethod(tpl, "toString", PCRE2Wrapper::ToString);
	
	v8::Local<v8::ObjectTemplate> proto = tpl->PrototypeTemplate();
	Nan::SetAccessor(proto, Nan::New("source").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("flags").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("lastIndex").ToLocalChecked(), PCRE2Wrapper::PropertyGetter, PCRE2Wrapper::PropertySetter);
	Nan::SetAccessor(proto, Nan::New("global").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("ignoreCase").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("multiline").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("sticky").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);

	constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
	exports->Set(Nan::GetCurrentContext(), Nan::New(name).ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

void PCRE2Wrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if ( !info.IsConstructCall() ) {
		Nan::ThrowError("Use `new` to create instances of this object.");
		return;
	}
	
	PCRE2Wrapper* obj = new PCRE2Wrapper();
	obj->Wrap(info.This());
	
	std::string pattern;
	std::string flags;
	
	if ( info[0]->IsString() ) {
		Nan::Utf8String nanPattern(Nan::To<v8::String>(info[0]).ToLocalChecked());
		pattern = std::string(*nanPattern, nanPattern.length());
		if (info[1]->IsUndefined()) {
			flags = "";
		} else {
			Nan::Utf8String nanFlags(Nan::To<v8::String>(info[1]).ToLocalChecked());
			flags = std::string(*nanFlags, nanFlags.length());
		}
	}
	else if ( info[0]->IsRegExp() ) {
		v8::Local<v8::RegExp> v8Regexp = info[0].As<v8::RegExp>();
		v8::Local<v8::String> v8Source = v8Regexp->GetSource();
		v8::RegExp::Flags v8Flags = v8Regexp->GetFlags();

		Nan::Utf8String nanV8Source(v8Source);
		pattern = std::string(*nanV8Source, nanV8Source.length());
		
		if ( bool(v8Flags & v8::RegExp::kIgnoreCase) ) flags += "i";
		if ( bool(v8Flags & v8::RegExp::kMultiline) ) flags += "m";
		if ( bool(v8Flags & v8::RegExp::kGlobal) ) flags += "g";
	}

	if( strchr(flags.c_str(), 'g') ) obj->global = true;

	Nan::Utf8String nanConstructorName(info.This()->GetConstructorName());
	std::string constructorName(*nanConstructorName, nanConstructorName.length());
	
	if( constructorName == "PCRE2JIT" ) {
		flags += "S";
	}
	
	obj->re.setPattern(pattern.c_str());
	obj->re.addModifier(flags.c_str());
	obj->re.compile();
	
	obj->flags = flags;
	
	if ( !obj->re ) {	
		Nan::ThrowError(obj->re.getErrorMessage().c_str());
		return;
	}

	info.GetReturnValue().Set(info.This());
}

void PCRE2Wrapper::Test(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());

	Nan::Utf8String nanSubject(info[0]);
	std::string subject(*nanSubject, nanSubject.length());
	
	jp::RegexMatch& rm = obj->rm;
	
	rm.clear();
	rm.setRegexObject(&obj->re);
	rm.setSubject(subject);
	rm.setModifier(obj->flags);

	rm.setStartOffset(obj->global ? obj->lastIndex : 0);
	rm.setMatchEndOffsetVector(&obj->vec_eoff);
	
	rm.setNumberedSubstringVector(NULL);
	rm.setNamedSubstringVector(NULL);
	rm.setNameToNumberMapVector(NULL);
	
	bool result = (rm.match() > 0);
	
	if ( result && obj->global ) obj->lastIndex = obj->vec_eoff[0];
	
	info.GetReturnValue().Set(result ? Nan::True() : Nan::False());
}

void PCRE2Wrapper::Exec(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());

	Nan::Utf8String nanSubject(info[0]);
	std::string subject(*nanSubject, nanSubject.length());
	jp::RegexMatch& rm = obj->rm;
	
	rm.clear();
	rm.setRegexObject(&obj->re);
	rm.setSubject(subject);
	rm.setModifier(obj->flags);
	
	rm.setStartOffset(obj->global ? obj->lastIndex : 0);
	rm.setMatchEndOffsetVector(&obj->vec_eoff);
	
	rm.setNumberedSubstringVector(&obj->vec_num);
	rm.setNamedSubstringVector(&obj->vec_nas);
	rm.setNameToNumberMapVector(&obj->vec_ntn);
	
	size_t result_count = rm.match();
	
	if ( result_count == 0 ) {
		obj->lastIndex = 0;
	
		info.GetReturnValue().Set(Nan::Null());
	}
	else {
		size_t finish_offset = obj->vec_eoff[0];
	
		if ( obj->global ) obj->lastIndex = finish_offset;
	
		v8::Local<v8::Array> result = Nan::New<v8::Array>(result_count);
		
		std::string whole_match = *obj->vec_num[0][0];
		
		for ( size_t i=0; i<obj->vec_num[0].size(); i++ ) {
			if (obj->vec_num[0][i])
				result->Set(Nan::GetCurrentContext(), i, Nan::New(*obj->vec_num[0][i]).ToLocalChecked());
			else
				result->Set(Nan::GetCurrentContext(), i, Nan::Null());
		}
				
		v8::Local<v8::Object> named = Nan::New<v8::Object>();
		result->Set(Nan::GetCurrentContext(), Nan::New("groups").ToLocalChecked(), named);
		result->Set(Nan::GetCurrentContext(), Nan::New("named").ToLocalChecked(), named);

		for ( auto const& ent : obj->vec_nas[0] ) {
			named->Set(Nan::GetCurrentContext(), Nan::New(ent.first).ToLocalChecked(), Nan::New(ent.second).ToLocalChecked());
		}
		
		int match_offset = finish_offset - whole_match.length();

		result->Set(Nan::GetCurrentContext(), Nan::New("index").ToLocalChecked(), Nan::New((int32_t)match_offset));
		result->Set(Nan::GetCurrentContext(), Nan::New("input").ToLocalChecked(), Nan::New(subject).ToLocalChecked());

		info.GetReturnValue().Set(result);
	}
}

void PCRE2Wrapper::Match(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	if ( !obj->global ) {
		PCRE2Wrapper::Exec(info);
		return;
	}
	
	Nan::Utf8String nanSubject(info[0]);
	std::string subject(*nanSubject, nanSubject.length());
	jp::RegexMatch& rm = obj->rm;
	
	rm.clear();
	rm.setRegexObject(&obj->re);
	rm.setSubject(subject);
	rm.setModifier("g");
	
	rm.setStartOffset(0);
	rm.setMatchEndOffsetVector(&obj->vec_eoff);

	rm.setNumberedSubstringVector(&obj->vec_num);
	rm.setNamedSubstringVector(NULL);
	rm.setNameToNumberMapVector(NULL);

	size_t result_count = rm.match();
	
	if ( result_count == 0 ) {
		obj->lastIndex = 0;
	
		info.GetReturnValue().Set(Nan::Null());
	}
	else {
		v8::Local<v8::Array> result = Nan::New<v8::Array>(result_count);
		
		for ( size_t i=0; i<obj->vec_num.size(); ++i ) {
			if (obj->vec_num[i][0])
				result->Set(Nan::GetCurrentContext(), i, Nan::New(*obj->vec_num[i][0]).ToLocalChecked());
			else
				result->Set(Nan::GetCurrentContext(), i, Nan::Null());
		}
		
		info.GetReturnValue().Set(result);
	}
}

void PCRE2Wrapper::Replace(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());

	Nan::Utf8String nanSubject(info[0]);
	std::string subject(*nanSubject, nanSubject.length());

	bool withCallback = info[1]->IsFunction();
	
	if ( !withCallback ) {
		Nan::Utf8String nanReplacement(info[1]);
		std::string replacement(*nanReplacement, nanReplacement.length());
		
		jp::RegexReplace & rr = obj->rr;
	
		rr.clear();
		rr.setRegexObject(&obj->re);
		rr.setSubject(subject);
		rr.setModifier(obj->flags);
		
		rr.setStartOffset(0);

		rr.setReplaceWith(replacement);
		
		std::string result = rr.replace();
		
		info.GetReturnValue().Set(Nan::New(result).ToLocalChecked());
	}
	else {
		v8::Local<v8::Function> callback = info[1].As<v8::Function>();
		
		jp::MatchEvaluator & me = obj->me;
	
		me.clear();
		me.setRegexObject(&obj->re);
		me.setSubject(subject);
		me.setModifier(obj->flags);

		me.setStartOffset(0);
		
		me.setCallback(
			[&me, &obj, &subject, &callback](const jp::NumSub& cg, const jp::MapNas& ng, void*) mutable {
				jp::VecNum const* vec_num = me.getNumberedSubstringVector();
				ptrdiff_t pos = distance(vec_num->begin(), find(vec_num->begin(), vec_num->end(), cg));
				
				const jpcre2::VecOff* vec_soff = me.getMatchStartOffsetVector();
				size_t match_offset = (*vec_soff)[pos];
				
				const unsigned argCount = 3 + cg.size();
				v8::Local<v8::Value> *argVector = new v8::Local<v8::Value>[argCount];

				for ( size_t i=0; i<cg.size(); i++ ) {
					if (cg[i]) //match, p1, p2, ... , pn
						argVector[i] = Nan::New(*cg[i]).ToLocalChecked();
					else
						argVector[i] = Nan::Null();
				}

				argVector[argCount-3] = Nan::New((uint32_t)match_offset); //offset
				argVector[argCount-2] = Nan::New(subject).ToLocalChecked(); //subject
				
				v8::Local<v8::Object> named = Nan::New<v8::Object>();
				argVector[argCount-1] = named; //named
				
				for ( auto const& ent : ng ) {
					named->Set(Nan::GetCurrentContext(), Nan::New(ent.first).ToLocalChecked(), Nan::New(ent.second).ToLocalChecked());
				}

				Nan::Utf8String retval(Nan::Callback(callback).Call(argCount, argVector));
				std::string returned(*retval, retval.length());
				delete[] argVector;
				return returned;
			}
		);

		{
			jpcre2::Uint po = 0, jo = 0;
			int en = 0;
			jpcre2::SIZE_T eo = 0;
			jpcre2::MOD::toReplaceOption(obj->flags, true, &po, &jo, &en, &eo);
			std::string result = me.nreplace(true, po);
			info.GetReturnValue().Set(Nan::New(result).ToLocalChecked());
		}
	}
}

void PCRE2Wrapper::ToString(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	std::string result = "/" + obj->re.getPattern() + "/" + obj->flags;
	
	info.GetReturnValue().Set(Nan::New(result).ToLocalChecked());
}

void PCRE2Wrapper::HasModifier(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	Nan::Utf8String nanWanted(info[0]);
	std::string wanted(*nanWanted, nanWanted.length());
	
	bool result = true;
	
	for(char& c : wanted) {
		if ( obj->flags.find(c) == std::string::npos ) {
			result = false;
			break;
		}
	}
	
	info.GetReturnValue().Set(result ? Nan::True() : Nan::False());
}

void PCRE2Wrapper::PropertyGetter(v8::Local<v8::String> property, const Nan::PropertyCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	Nan::Utf8String nanProperty(property);
	std::string name(*nanProperty, nanProperty.length());
	
	if ( name == "source" ) {
		info.GetReturnValue().Set(Nan::New(obj->re.getPattern()).ToLocalChecked());
	}
	else if ( name == "flags" ) {
		std::string modifier = obj->re.getModifier();
		if ( obj->global ) modifier += "g";
		
		info.GetReturnValue().Set(Nan::New(modifier).ToLocalChecked());
	}
	else if ( name == "lastIndex" ) {
		info.GetReturnValue().Set(Nan::New<v8::Integer>((uint32_t)obj->lastIndex));
	}
	else {
		bool result = false;
		
		if ( name == "global" ) result = (obj->flags.find('g') != std::string::npos);
		else if ( name == "sticky" ) result = false;
		else if ( name == "ignoreCase" ) result = (obj->flags.find('i') != std::string::npos);
		else if ( name == "multiline" ) result = (obj->flags.find('m') != std::string::npos);
		
		info.GetReturnValue().Set(Nan::New(result));
	}
}

NAN_SETTER(PCRE2Wrapper::PropertySetter) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	Nan::Utf8String nanProperty(property);
	std::string name(*nanProperty, nanProperty.length());
	
	if ( name == "lastIndex" ) {
		int32_t val = Nan::To<int32_t>(value).FromJust();
		obj->lastIndex = val < 0 ? 0 : val;
		info.GetReturnValue().Set(Nan::New<v8::Integer>((uint32_t)obj->lastIndex));
	}
}
