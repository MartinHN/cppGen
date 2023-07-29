import assert from "assert";
import { exit } from "process";
import diff from 'deep-diff';


// const diff = diffM.diff
/// obj helpers
export function jsonReplacer(key: string, o: any) {
    if (typeof o == "bigint")
        return "__bigInt:" + o.toString();
    return o;
}

export function jsonReviver(key: string, o: any) {
    if (typeof o == "string" && o.startsWith("__bigInt:"))
        return BigInt(o.replace("__bigInt:", ""));
    return o;
}
export function printJsObj(obj: any) {
    console.log("obj\n", JSON.stringify(obj, jsonReplacer, 2))
}

export function getObjSnapshot(obj: any) {
    return JSON.parse(JSON.stringify(obj, jsonReplacer), jsonReviver)
}


// test definitions
type testFun = (() => void);
type testEntry = { name: string, function: testFun, numAssert: number, numAssertsFailed: number }
const allTests = {} as { [id: string]: testEntry }
let runningTest: testEntry;
export function test(name: string, f: testFun) {
    allTests[name] = { name, function: f, numAssert: 0, numAssertsFailed: 0 };
}


// assert helpers
export function expect(o: any) {
    runningTest.numAssert++;
    if (!o) {
        runningTest.numAssertsFailed++;
        throw new Error("expect failed" + runningTest.name);
    }
    return o;
}

export function expectDeepEqual(a: any, b: any) {

    const d = diff(a, b)
    try {
        expect(d === undefined);
    }
    catch (e) {
        console.log(d)
        throw new Error("not equal")
    }

}


// run helpers
export function runAll(exitOnFailure = true) {
    let totalAssertsSucceeded = 0;
    let totalAssertsFailed = 0;
    for (const [k, v] of Object.entries(allTests)) {
        runningTest = v;
        console.log(" +  starting test", k)
        try {
            v.function();
        }
        catch (e) {
            console.error(" + test failed", k)
            console.error(e)
            if (exitOnFailure)
                exit()
        }

        console.log(" + test eneded", k, "with", runningTest.numAssertsFailed, "failed asserts")
        totalAssertsSucceeded += runningTest.numAssert
        totalAssertsFailed += runningTest.numAssertsFailed
    }

    console.log(" + test done : ", totalAssertsSucceeded, " asserts have succeeded")

}
