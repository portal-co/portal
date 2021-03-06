{
module Lexer (Token (..), LexOut (..), scan) where

import Representation (Nat)
}

%wrapper "posn"

@char = [a-zA-Z0-9\$]
-- @digit = [0-9]

tokens :-

  $white+                               ;
  "#".*                                 ;

  "("                                   { \p s -> lexOut p LLParen }
  ")"                                   { \p s -> lexOut p LRParen }

  "->"                                  { \p s -> lexOut p LArrow }

  ":"                                   { \p s -> lexOut p LColon }

  "."                                   { \p s -> lexOut p LDot }

  "="                                   { \p s -> lexOut p LEqual }

  "@"                                  { \p s -> lexOut p LForall }
  "∀"                                   { \p s -> lexOut p LForall }

  "\"                                   { \p s -> lexOut p LLambda }
  "λ"                                   { \p s -> lexOut p LLambda }

  "axiom"                               { \p s -> lexOut p LAxiom }
  "let"                                 { \p s -> lexOut p LLet }

  "*"                                   { \p s -> lexOut p LStar }

  "_"                                   { \p s -> lexOut p LUnderscore }

  -- @digit+                               { \p s -> lexOut p (LNumber (read s)) }
  @char+                                { \p s -> lexOut p (LSym s) }

{
data Token = LLParen
           | LRParen
           | LArrow
           | LColon
           | LDot
           | LEqual
           | LForall
           | LLambda
           | LAxiom
           | LLet
           | LStar
           | LUnderscore
           -- | LNumber Nat
           | LSym String
           deriving (Eq, Show)

data LexOut = LexOut { offset :: Int
                     , line :: Int
                     , column :: Int
                     , getToken :: Token }
              deriving (Eq, Show)

lexOut (AlexPn offset line col) tok = LexOut offset line col tok

scan = alexScanTokens
}
