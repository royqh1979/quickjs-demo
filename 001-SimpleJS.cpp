/**
 * 执行JS脚本，获取变量的值。
 */
#include <quickjs.h>
#include <stdio.h>
#include <stdint.h>

void fatal(const char* msg);
void js_dump_obj(JSContext* ctx, JSValue obj);
void js_dump_exception(JSContext* ctx);

int main() {
	//创建引擎和语境对象
	JSRuntime *rt = JS_NewRuntime();
	if (rt==NULL)
		fatal("Can't create js runtime!");
	JSContext *ctx = JS_NewContext(rt);
	if (ctx==NULL) {
		JS_FreeRuntime(rt);
		fatal("Can't create js context!");
	}
	
	//在语境中执行Javascript脚本
	const char *foo_code = "var test = 5; test = test + 1;";
	JSValue evalResult = JS_Eval(ctx, foo_code, sizeof(foo_code), "<input>", JS_EVAL_FLAG_STRICT);
	//判断脚本是否成功执行
	if (JS_IsException(evalResult)) {
		js_dump_exception(ctx);
		
		JS_FreeContext(ctx);
		JS_FreeRuntime(rt);	
		fatal("Failed to eval foo()!");
	}
	
	//通过全局对象获取test变量的值
	JSValue global = JS_GetGlobalObject(ctx);
	JSValue testVar = JS_GetPropertyStr(ctx, global, "test");
	int32_t result;
	JS_ToInt32(ctx, &result, testVar);
	printf("Result: %d\n",result);
	
	//清理
	JS_FreeValue(ctx,testVar);
	JS_FreeValue(ctx,global);
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);	
}

/**
 * @brief 显示错误提示并结束程序
 * 
 * @param msg 
 */
void fatal(const char* msg) {
	printf("Fatal: %s\n",msg);
	exit(-1);
}

/**
 * @brief 显示JS值内的字符串内容
 * 
 * @param ctx 
 * @param obj 
 */
void js_dump_obj(JSContext* ctx, JSValue obj) {
	const char *str = JS_ToCString(ctx, obj);
	if (str) {
		printf("%s\n", str);
		JS_FreeCString(ctx, str);	
	} else 
		printf("[Exception]\n");
}

/**
 * @brief 显示JS异常的具体内容
 * 
 * @param ctx 
 */
void js_dump_exception(JSContext* ctx) {
	JSValue exception = JS_GetException(ctx);
	js_dump_obj(ctx, exception);
	if (JS_IsError(ctx, exception)) {
		JSValue val = JS_GetPropertyStr(ctx, exception, "stack");
		if (!JS_IsUndefined(val)) {
			js_dump_obj(ctx, val);
		}
		JS_FreeValue(ctx, val);
	}	
}
