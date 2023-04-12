import * as fs from 'fs'

class MemberType {
    constructor(public type: string, public name: string) { }
}

type MembersCollection = MemberType[];

class MethodType {
    constructor(public returnType: string, public name: string, public args: MemberType[]) {
    }

    hasArgs() {
        return this.args && (this.args.length > 0)
    }
    getArgTypeList() {
        return this.args.map(e => e.type);
    }
    getFunctionArgsWithVariables() {
        return this.getArgTypeList().map((e, i) => { return e + " " + this.getArgVarName(i) });
    }

    getArgVarName(i: number) {
        return "inArg_" + i;
    }
    getArgVarNames() {
        const res = []
        for (let i = 0; i < this.args.length; i++) {
            res.push(this.getArgVarName(i));
        }
        return res;
    }
}


function isObject(o: any): boolean {
    return typeof o === 'object' &&
        !Array.isArray(o) &&
        o !== null
}


function makeFullType(m: MemberType) {
    m.type = m.type.trim()
    // for (const stdCl of ["vector", "map","array"])
    //     if (m.type.startsWith(stdCl))
    //         m.type = m.type.replace(stdCl, "std::" + stdCl)
}


export class APIClass {

    public isUserDefined = true;
    public members: MembersCollection = [];
    public methods: MethodType[] = [];
    constructor(public name: string, obj: any) {
        this.members = obj.members;
        this.members.map(makeFullType)
        this.methods = obj.methods.map((e: any) => { return new MethodType(e.returnType, e.name, e.args) })
    }

    getAllClasses() {
        const allT = new Array<string>()
        const getTypes = (o: any) => {
            if (o && o.type) {
                const t = o.type.trim();
                if (!allT.includes(t)) allT.push(t)
            }

            if (isObject(o) || Array.isArray(o))
                for (const v of Object.values(o)) {
                    getTypes(v);
                }
        }
        getTypes(this);
        return allT;
    }

    getAllIds() {
        // should not have name clashes with members or functions
        // maybe with func args if we need them at some point???
        return this.members.map(e => e.name)
    }

};






export class TypeSystem {
    public classes = new Array<APIClass>();
    public metadata: any = {}


    getAllClasses() {
        const allT = new Array<string>()
        this.classes.map(c => {
            if (!allT.includes(c.name)) allT.push(c.name)
            c.getAllClasses().map(cc => { if (!allT.includes(cc)) allT.push(cc) })
        })
        return allT;
    }

    getAllIds() {
        const res: string[] = []
        for (const c of this.classes)
            for (const i of c.getAllIds())
                if (!res.includes(i))
                    res.push(i)
        return res;
    }

    getMembersForClass(className: string) {
        for (const c of this.classes)
            if (c.name == className)
                return c.members

        return []
    }

    getMethodsForClass(className: string) {
        for (const c of this.classes)
            if (c.name == className)
                return c.methods

        return []
    }
    isClassUserDefined(className: string) {
        for (const c of this.classes)
            if (c.name == className)
                return true

        return false;
    }

}

export function load(filename: string): TypeSystem {
    const obj = JSON.parse(fs.readFileSync(filename).toString());
    const ts = new TypeSystem();
    for (const [k, v] of Object.entries(obj)) {
        if (k.startsWith("meta"))
            ts.metadata = v;
        else
            ts.classes.push(new APIClass(k, v))
    }

    // try to put lightweight class first to avoid dependency hell
    const allC = ts.classes.map(e => e.name)
    function getNumOfIncludedClass(c: APIClass) {
        let count = 0;
        for (const m of c.members)
            if (allC.find(e => m.name.includes(e)) !== undefined)
                count++;
        return count;
    }
    // console.log(ts.classes.map(e=>e.name))
    ts.classes = ts.classes.sort((a, b) => { return getNumOfIncludedClass(b) - getNumOfIncludedClass(a) })
    // console.log(ts.classes.map(e=>e.name))
    return ts;
}

