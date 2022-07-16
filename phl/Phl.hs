{-# LANGUAGE RankNTypes #-}
module Phl where
import Barbies
import Control.Monad.Cont (MonadTrans)
newtype Comt g a = Comt {getComt :: forall r. Monoid r => Compose (Cont r) g a}
instance MonadT Comt where
    tlift = Comt . tlift
    tjoin = coerce . tjoin . coerce

merg = bmap (join . getCompose)

companic = Comt $ cont (const mempty)
comfork = Comt $ cont (\cc -> cc True `mappend` cc False)
combranch arr = Cont $ cont (\cc -> mconcat $ fmap cc arr)
class IxMonad m where
    ireturn :: a -> m p p a
    ibind :: m p q a -> (a -> m q r b) -> m p r b

newtype IxWrap w m p q a = IxWrap {unIxWrap :: w (m p q) a}
instance (IxMonad m, MonadT w) => IxMonad (IxWrap w m) where
    ireturn x = IxWrap $ tlift $ ireturn x
    ibind (IxWrap m) f = flip ibind id <$> tembed (ibind f)

type IxCont r m p q a = IxWrap (Compose (Cont r)) m p q a
newtype IxStateT m si so v = IxStateT { runIxStateT:: si -> m (so,v) }

instance Monad m => IxMonad (IxStateT m) where
  ireturn x = IxStateT (\si -> return (si,x))
  ibind (IxStateT m) f = IxStateT (\si -> m si >>= (\ (sm,x) -> runIxStateT (f x) sm))

vsget :: Monad m => IxStateT m si si si
vsget = IxStateT (\si -> return (si,si))

vsput :: Monad m => so -> IxStateT m si so ()
vsput x = IxStateT (\si -> return (x,()))
