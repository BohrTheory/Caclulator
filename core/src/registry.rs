//! Immutable source registry; adding language features requires an accepted contract.
use crate::{Result, operations::basic};
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Fixity {
    Prefix,
    Infix,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Associativity {
    Left,
    Right,
}
// Stable metadata is validated by registry unit tests; it is not a runtime plugin API.
#[allow(dead_code)]
pub struct Operation {
    pub key: &'static str,
    pub token: u8,
    pub arity: usize,
    pub fixity: Fixity,
    pub binding_power: u8,
    pub associativity: Associativity,
    pub module: &'static str,
    pub evaluate: fn(f64, f64, usize) -> Result<f64>,
}
pub static OPERATIONS: &[Operation] = &[
    Operation {
        key: "add",
        token: b'+',
        arity: 2,
        fixity: Fixity::Infix,
        binding_power: 1,
        associativity: Associativity::Left,
        module: "basic",
        evaluate: basic::add,
    },
    Operation {
        key: "subtract",
        token: b'-',
        arity: 2,
        fixity: Fixity::Infix,
        binding_power: 1,
        associativity: Associativity::Left,
        module: "basic",
        evaluate: basic::subtract,
    },
    Operation {
        key: "multiply",
        token: b'*',
        arity: 2,
        fixity: Fixity::Infix,
        binding_power: 2,
        associativity: Associativity::Left,
        module: "basic",
        evaluate: basic::multiply,
    },
    Operation {
        key: "divide",
        token: b'/',
        arity: 2,
        fixity: Fixity::Infix,
        binding_power: 2,
        associativity: Associativity::Left,
        module: "basic",
        evaluate: basic::divide,
    },
    Operation {
        key: "positive",
        token: b'+',
        arity: 1,
        fixity: Fixity::Prefix,
        binding_power: 3,
        associativity: Associativity::Right,
        module: "basic",
        evaluate: basic::positive,
    },
    Operation {
        key: "negative",
        token: b'-',
        arity: 1,
        fixity: Fixity::Prefix,
        binding_power: 3,
        associativity: Associativity::Right,
        module: "basic",
        evaluate: basic::negative,
    },
];
pub fn find(token: u8, fixity: Fixity) -> Option<&'static Operation> {
    OPERATIONS
        .iter()
        .find(|op| op.token == token && op.fixity == fixity)
}
