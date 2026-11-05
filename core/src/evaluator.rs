use crate::{Result, abi_generated::*, error, parser::Instruction};
pub fn evaluate(program: &[Instruction<'_>]) -> Result<f64> {
    let mut stack = Vec::new();
    for instruction in program {
        match instruction {
            Instruction::Literal(text, pos) => {
                let value = text.parse::<f64>().map_err(|_| error(CALC_RANGE, *pos))?;
                if !value.is_finite() {
                    return Err(error(CALC_RANGE, *pos));
                }
                stack.push(value);
            }
            Instruction::Apply(op, pos) => {
                let right = stack.pop().expect("validated operand");
                let (a, b) = if op.arity == 2 {
                    (stack.pop().expect("validated left operand"), right)
                } else {
                    (right, 0.0)
                };
                stack.push((op.evaluate)(a, b, *pos)?);
            }
        }
    }
    Ok(stack.pop().expect("validated expression"))
}
