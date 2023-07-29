import { execSync, spawnSync } from "child_process";
import * as fs from "fs"
import * as path from "path"


export function safeClean(of: string) {
    if (of.length < 10) {
        throw Error("folder path to clean is too short?" + of)
    }
    if (fs.existsSync(of)) { execSync("rm -r " + of); }
    execSync("mkdir -p " + of);
}


export function safeCpFolder(inF: string, outF: string) {
    console.log("will copy " + inF + " to " + outF)
    if (path.dirname(inF) == path.dirname(outF)) {
        console.log("not copying to same dir")
        return;
    }
    // safeClean(outF)
    spawnSync("cp -rf " + inF + " " + outF, { shell: true, stdio: 'inherit' });
}

export function mkSymLink(inF: string, outF: string) {
    console.log("will symlink " + inF + " to " + outF)
    if (path.dirname(inF) == path.dirname(outF)) {
        console.log("not symlinking to same dir")
        return;
    }

    // safeClean(outF)
    execSync("ln -sf " + inF + " " + outF);
}


export function rsync(from: string, to: string) {
    console.log(`will rsync  ${from} to ${to}`)
    if (!from.endsWith("/"))
        from = from + "/"
    if (!to.endsWith("/"))
        to = to + "/"

    spawnSync(`rsync -r --checksum ${from} ${to}; echo "done copying ${to}"`, { shell: true, stdio: 'inherit' });
}
