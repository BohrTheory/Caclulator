//! Bounded recursive descent emits a flat postfix program, never a recursive AST.
use crate::{
    Result,
    abi_generated::*,
    error,
    registry::{self, Associativity, Fixity, Operation},
};
pub enum Instruction<'a> {
    Literal(&'a str, usize),
    Apply(&'static Operation, usize),
}
pub fn parse(text: &str) -> Result<Vec<Instruction<'_>>> {
    let mut parser = Parser {
        text,
        pos: 0,
        program: Vec::new(),
    };
    parser.expression(0, 1)?;
    parser.space();
    if parser.pos != text.len() {
        return Err(error(CALC_SYNTAX, parser.pos));
    }
    Ok(parser.program)
}
struct Parser<'a> {
    text: &'a str,
    pos: usize,
    program: Vec<Instruction<'a>>,
}
impl<'a> Parser<'a> {
    fn peek(&self) -> Option<u8> {
        self.text.as_bytes().get(self.pos).copied()
    }
    fn space(&mut self) {
        while matches!(self.peek(), Some(b' ' | b'\t' | b'\r' | b'\n')) {
            self.pos += 1;
        }
    }
    fn descend(&self, depth: usize) -> Result<usize> {
        if depth >= CALC_MAX_PARSE_DEPTH as usize {
            Err(error(CALC_LIMIT, self.pos))
        } else {
            Ok(depth + 1)
        }
    }
    fn expression(&mut self, depth: usize, min: u8) -> Result<()> {
        self.unary(depth)?;
        loop {
            self.space();
            let Some(op) = self.peek().and_then(|b| registry::find(b, Fixity::Infix)) else {
                break;
            };
            if op.binding_power < min {
                break;
            }
            let pos = self.pos;
            self.pos += 1;
            let next = op.binding_power + u8::from(op.associativity == Associativity::Left);
            self.expression(depth, next)?;
            self.program.push(Instruction::Apply(op, pos));
        }
        Ok(())
    }
    fn unary(&mut self, depth: usize) -> Result<()> {
        self.space();
        if let Some(op) = self.peek().and_then(|b| registry::find(b, Fixity::Prefix)) {
            let next = self.descend(depth)?;
            let pos = self.pos;
            self.pos += 1;
            self.unary(next)?;
            self.program.push(Instruction::Apply(op, pos));
            return Ok(());
        }
        if self.peek() == Some(b'(') {
            let next = self.descend(depth)?;
            self.pos += 1;
            self.expression(next, 1)?;
            self.space();
            if self.peek() != Some(b')') {
                return Err(error(CALC_SYNTAX, self.pos));
            }
            self.pos += 1;
            return Ok(());
        }
        self.number()
    }
    fn digits(&mut self) -> usize {
        let start = self.pos;
        while self.peek().is_some_and(|b| b.is_ascii_digit()) {
            self.pos += 1;
        }
        self.pos - start
    }
    fn number(&mut self) -> Result<()> {
        let start = self.pos;
        let mut digits = self.digits();
        if self.peek() == Some(b'.') {
            self.pos += 1;
            digits += self.digits();
        }
        if digits == 0 {
            return Err(error(CALC_SYNTAX, start));
        }
        if matches!(self.peek(), Some(b'e' | b'E')) {
            self.pos += 1;
            if matches!(self.peek(), Some(b'+' | b'-')) {
                self.pos += 1;
            }
            if self.digits() == 0 {
                return Err(error(CALC_SYNTAX, self.pos));
            }
        }
        self.program
            .push(Instruction::Literal(&self.text[start..self.pos], start));
        Ok(())
    }
}
