-- TTyped: A dependently typed programming language.
-- Copyright (C) 2018  Taran Lynn
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <https://www.gnu.org/licenses/>.


module Representation where

import Data.List (findIndex, intersperse)
import Data.Word


type Nat = Word64


{- The strings in Quant, Var, Prod, and Fun all represent variable names.
   However, since variables use de Bruijn indices, these variable names are only
   used for debugging purposes.
-}


data Term = C Context
          | O Object
          deriving (Eq, Show)


data Context = Star
             | Quant (Maybe String) Term Context
             deriving (Eq, Show)

contextLength :: Context -> Nat
contextLength Star = 0
contextLength (Quant _ _ c) = 1 + contextLength c

concatContext :: Context -> Context -> Context
concatContext Star c = c
concatContext (Quant name t c1) c2 = Quant name t (concatContext c1 c2)

concatTerm :: Context -> Maybe String -> Term -> Context
concatTerm c name t = concatContext c (Quant name t Star)

mapContext :: (Term -> Term) -> Context -> Context
mapContext f Star = Star
mapContext f (Quant name t c) = Quant name (f t) (mapContext f c)


-- Axioms always have an associate string, because they're symbolic
-- objects.
data Object = Var String Nat
            | Prod (Maybe String) Term Object
            | Fun (Maybe String) Term Object
            | App Object Object
            | Axiom String Term
            deriving (Eq, Show)


-- | Increments all free variables by some amount.
addTerm :: Nat -> Term -> Term
addTerm n t = addTerm' n t 0

addContext :: Nat -> Context -> Context
addContext n t = addContext' n t 0

addObject :: Nat -> Object -> Object
addObject n t = addObject' n t 0


addTerm' n (C c) idx = C (addContext' n c idx)
addTerm' n (O o) idx = O (addObject' n o idx)


addContext' _ Star _ = Star
addContext' n (Quant name t c) idx =
  Quant name (addTerm' n t idx) (addContext' n c (idx + 1))


addObject' n (Var name index) idx =
  if index < idx then Var name index else Var name (index + n)
addObject' n (Prod name t o) idx =
  Prod name (addTerm' n t idx) (addObject' n o (idx + 1))
addObject' n (Fun name t o) idx =
  Fun name (addTerm' n t idx) (addObject' n o (idx + 1))
addObject' n (App o1 o2) idx = App (addObject' n o1 idx) (addObject' n o2 idx)
addObject' _ ax@(Axiom _ _) _ = ax



-- * Pretty Printing

ppTerm :: Term -> String
ppTerm = ppTerm' [] False

ppContext :: Context -> String
ppContext = ppContext' [] False

ppObject :: Object -> String
ppObject = ppObject' [] False

{-
   For all the following functions the first argument is a list of bound
   variables whose respective indices correspond to the de Bruijn index, and the
   second argument determines whether the term should be explicitly
   parenthesized.
-}

ppTerm' :: [String] -> Bool -> Term -> String
ppTerm' vars expParen (C c) = ppContext' vars expParen c
ppTerm' vars expParen (O o) = ppObject' vars expParen o

-- TODO: Remove duplication between pretty print Quant, Prod, and Fun.

ppContext' :: [String] -> Bool -> Context -> String
ppContext' _ _ Star = "*"
ppContext' vars expParen (Quant Nothing t c) =
  let s = ppTerm' vars True t
  in maybeParen expParen (ppQuant (unusedName:vars) [s] c)
ppContext' vars expParen (Quant (Just name) t c) =
  let s = "???" ++ name ++ " : " ++ ppTerm' vars True t ++ ". "
        ++ ppContext' (name:vars) False c
  in maybeParen expParen s

ppObject' :: [String] -> Bool -> Object -> String
ppObject' vars _ (Var name index) =
  -- Print a variable along with its de Bruijn index if the variable does not
  -- refer to the closest binding with the same name.
  if maybe 0 fromIntegral (findIndex (== name) vars) /= index
  then name ++ "[" ++ show index ++ "]"
  else name
ppObject' vars expParen (Prod Nothing t o) =
  let s = ppTerm' vars True t
  in maybeParen expParen (ppFunType (unusedName:vars) [s] o)
ppObject' vars expParen (Prod (Just name) t o) =
  let s = "???" ++  name++ " : " ++ ppTerm' vars True t ++ ". "
        ++ ppObject' (name:vars) False o
  in maybeParen expParen s
ppObject' vars expParen (Fun name t o) =
  let name' = ppArgName name
      s = "??" ++ name' ++ " : " ++ ppTerm' vars True t ++ ". "
        ++ ppObject' (name':vars) False o
  in maybeParen expParen s
ppObject' vars expParen (App o1 o2) = maybeParen expParen (ppApps vars [o2] o1)
ppObject' vars expParen (Axiom name _) =
  if elem name vars then name ++ "[???]" else name


unusedName :: String
unusedName = "_"

ppArgName :: Maybe String -> String
ppArgName Nothing = unusedName
ppArgName (Just s) = s

maybeParen :: Bool -> String -> String
maybeParen False s = s
maybeParen True s = "(" ++ s ++ ")"


ppQuant vars ss (Quant Nothing t c) =
  let s = ppTerm' vars True t
  in ppQuant (unusedName:vars) (s:ss) c
ppQuant vars ss c = ppQ (reverse ss) ++ " -> " ++ ppContext' vars True c
  where
    ppQ ss = intersperse " -> " ss >>= id


ppFunType vars ss (Prod Nothing t o) =
  let s = ppTerm' vars True t
  in ppFunType (unusedName:vars) (s:ss) o
ppFunType vars ss o = ppFT (reverse ss) ++ " -> " ++ ppObject' vars True o
  where
    ppFT ss = intersperse " -> " ss >>= id


ppApps vars os (App o1 o2) = ppApps vars (o2:os) o1
ppApps vars os o = ppApps' (o:os)
  where
    ppApps' os = intersperse " " (map (ppObject' vars True) os) >>= id
