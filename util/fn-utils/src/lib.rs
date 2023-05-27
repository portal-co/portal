#![feature(specialization)]
use tuple_utils::Pluck;

pub trait Meld{
    type Output;
    fn meld(self) -> Self::Output;
}
impl Meld for (){
    type Output = ();
    fn meld(self) -> Self::Output {
        self
    }
}
impl <T: Pluck> Meld for T where <T as Pluck>::Tail: Meld, T::Head: tuple_utils::Merge<<<T as Pluck>::Tail as Meld>::Output>{
    default type Output = <T::Head as tuple_utils::Merge<<<T as Pluck>::Tail as Meld>::Output>>::Output;
    default fn meld(self) -> Self::Output {
        let (a,b) = self.pluck();
        return a.merge(b.meld());
    }
}
