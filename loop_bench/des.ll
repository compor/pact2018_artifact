; ModuleID = 'des.cpp'
source_filename = "des.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @_Z6serialiffPfS_(i32 %N, float %alpha, float %beta, float* %x, float* %y_seq) #0 {
entry:
  %N.addr = alloca i32, align 4
  %alpha.addr = alloca float, align 4
  %beta.addr = alloca float, align 4
  %x.addr = alloca float*, align 8
  %y_seq.addr = alloca float*, align 8
  %s = alloca float, align 4
  %b = alloca float, align 4
  %i = alloca i32, align 4
  %s_next = alloca float, align 4
  store i32 %N, i32* %N.addr, align 4
  store float %alpha, float* %alpha.addr, align 4
  store float %beta, float* %beta.addr, align 4
  store float* %x, float** %x.addr, align 8
  store float* %y_seq, float** %y_seq.addr, align 8
  %0 = load float*, float** %x.addr, align 8
  %arrayidx = getelementptr inbounds float, float* %0, i64 1
  %1 = load float, float* %arrayidx, align 4
  store float %1, float* %s, align 4
  %2 = load float*, float** %x.addr, align 8
  %arrayidx1 = getelementptr inbounds float, float* %2, i64 1
  %3 = load float, float* %arrayidx1, align 4
  %4 = load float*, float** %x.addr, align 8
  %arrayidx2 = getelementptr inbounds float, float* %4, i64 0
  %5 = load float, float* %arrayidx2, align 4
  %sub = fsub float %3, %5
  store float %sub, float* %b, align 4
  store i32 2, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %6 = load i32, i32* %i, align 4
  %7 = load i32, i32* %N.addr, align 4
  %cmp = icmp slt i32 %6, %7
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %8 = load float, float* %alpha.addr, align 4
  %9 = load float*, float** %x.addr, align 8
  %10 = load i32, i32* %i, align 4
  %idxprom = sext i32 %10 to i64
  %arrayidx3 = getelementptr inbounds float, float* %9, i64 %idxprom
  %11 = load float, float* %arrayidx3, align 4
  %mul = fmul float %8, %11
  %12 = load float, float* %alpha.addr, align 4
  %sub4 = fsub float 1.000000e+00, %12
  %13 = load float, float* %s, align 4
  %14 = load float, float* %b, align 4
  %add = fadd float %13, %14
  %mul5 = fmul float %sub4, %add
  %add6 = fadd float %mul, %mul5
  store float %add6, float* %s_next, align 4
  %15 = load float, float* %beta.addr, align 4
  %16 = load float, float* %s_next, align 4
  %17 = load float, float* %s, align 4
  %sub7 = fsub float %16, %17
  %mul8 = fmul float %15, %sub7
  %18 = load float, float* %beta.addr, align 4
  %sub9 = fsub float 1.000000e+00, %18
  %19 = load float, float* %b, align 4
  %mul10 = fmul float %sub9, %19
  %add11 = fadd float %mul8, %mul10
  store float %add11, float* %b, align 4
  %20 = load float, float* %s_next, align 4
  store float %20, float* %s, align 4
  %21 = load float, float* %s, align 4
  %22 = load float, float* %b, align 4
  %add12 = fadd float %21, %22
  %23 = load float*, float** %y_seq.addr, align 8
  %24 = load i32, i32* %i, align 4
  %idxprom13 = sext i32 %24 to i64
  %arrayidx14 = getelementptr inbounds float, float* %23, i64 %idxprom13
  store float %add12, float* %arrayidx14, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %25 = load i32, i32* %i, align 4
  %inc = add nsw i32 %25, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0 (trunk 321123)"}
