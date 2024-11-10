/**
 * 创建简单的js对象：
 * Summer = {
 *   'initial':0,
 *   'sum': function(...) 
 * }
 * 用C语言定义其普通方法
 */
#include <quickjs.h>
#include <stdio.h>
#include <stdint.h>

void fatal(const char* msg);
void js_dump_obj(JSContext* ctx, JSValue obj);
void js_dump_exception(JSContext* ctx);


/**
 * @brief sum函数
 * 
 * @param ctx		js语境 
 * @param this_val	this对象
 * @param argc		参数的个数
 * @param argv		参数数组
 * 
 * @return 
 */
JSValue sum(JSContext *ctx, JSValue this_val, int argc, JSValue* argv) {
	//确保this对象合法
	if (!JS_IsObject(this_val)) {
		return JS_ThrowTypeError(ctx,"'this' is not an object.");
	}
	//从this对象中获取initial属性的值，保存到result变量中
	JSValue initialVal = JS_GetPropertyStr(ctx,this_val,"initial");
	if (JS_IsException(initialVal)) {
		return initialVal;
	}
	int32_t result;
	if (JS_ToInt32(ctx, &result, initialVal) == -1)
		return JS_EXCEPTION;
	
	//将各实参的值累加到result变量上
	for (int i=0;i<argc;i++) {
		int32_t a;
		if (JS_ToInt32(ctx, &a, argv[i]) == -1) 
			return JS_EXCEPTION;
		result+=a;
	}
	
	//将result转换成JSValue后返回
	return JS_NewInt32(ctx, result);
}

int main() {
	//创建js引擎和语境对象
	JSRuntime *rt = JS_NewRuntime();
	if (rt==NULL)
		fatal("Can't create js runtime!");
	JSContext *ctx = JS_NewContext(rt);
	if (ctx==NULL)
		fatal("Can't create js context!");

	//创建sumObj对象
	JSValue sumObj =JS_NewObject(ctx);
	//将0赋给sumObj的"initial"属性
	JSValue initialVal = JS_NewInt32(ctx,0);
	JS_SetPropertyStr(ctx, sumObj, "initial", initialVal);
	
	//创建sum函数句柄并将其赋成sumObj的sum方法
	JSValue sumFunc = JS_NewCFunction(ctx,sum,"sum function",2);
	JS_SetPropertyStr(ctx, sumObj, "sum", sumFunc);
	
	//通过语境的全局对象，将sumObj赋给"Summber"变量
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx,global,"Summer",sumObj);
	
	//执行js脚本
	const char *foo_code = "Summer.initial=10; var result = Summer.sum(1,2,3,4,5);";
	JSValue evalResult = JS_Eval(ctx, foo_code, sizeof(foo_code), "<input>", JS_EVAL_FLAG_UNUSED);
	//如果执行不成功，显示错误信息
	if (JS_IsException(evalResult)) {
		js_dump_exception(ctx);
		JS_FreeContext(ctx);
		JS_FreeRuntime(rt);	
		fatal("Failed to eval foo()!");
	}

	//获取result变量的值
	JSValue jsResult = JS_GetPropertyStr(ctx, global, "result");
	int32_t result;
	JS_ToInt32(ctx, &result, jsResult);
	printf("Result: %d\n",result);
	
	//清理
	JS_FreeValue(ctx, jsResult);
	JS_FreeValue(ctx, initialVal);
	JS_FreeValue(ctx, sumFunc);
	JS_FreeValue(ctx, global);
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);	
	return 0;
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
