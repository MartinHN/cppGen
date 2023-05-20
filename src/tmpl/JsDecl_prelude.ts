type ArrayLengthMutationKeys = 'splice' | 'push' | 'pop' | 'shift' | 'unshift'
export type FixedLengthArray<T, L extends number, TObj = [T, ...Array<T>]> =
  Pick<TObj, Exclude<keyof TObj, ArrayLengthMutationKeys>>
  & {
    readonly length: L
    [I: number]: T
    [Symbol.iterator]: () => IterableIterator<T>
  }


export class ModifiableArray<T> extends Array<T> {
  constructor(public cf: any) {
    super()
  }
  emplace(args: any[]) {
    const newO = new this.cf();
    console.log("emplace", newO);
    this.push(newO)
  }
}
