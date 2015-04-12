export interface IComputeStatistics {
    latest : number;
    initialize(seedData : number[]) : void;
    addNewValue(value : number) : number;
}

export class EwmaStatisticCalculator implements IComputeStatistics {
    private _latest : number = null;
    public get latest() { return this._latest; }

    initialize(seedData: number[]): IComputeStatistics {
        seedData.forEach(this.addNewValue);
        return this;
    }

    addNewValue(value: number): number {
        this._latest = computeEwma(value, this._latest, this._alpha);
        return this._latest;
    }

    constructor(private _alpha : number) {}
}

export function computeEwma(newValue: number, previous: number, alpha: number) : number {
    if (previous !== null) {
        return alpha * newValue + (1 - alpha) * previous;
    }

    return newValue;
}