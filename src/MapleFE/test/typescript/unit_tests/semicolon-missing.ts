class Klass {}

export interface Interf {
    [key: string]: any;
}

export interface Interf2{
    func(component: Klass): Interf
    func2(component: Klass): Interf
}
