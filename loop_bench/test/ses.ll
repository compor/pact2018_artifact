; ModuleID = 'ses.cpp'
source_filename = "ses.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @_Z6serialifPfS_(i32 %N, float %alpha, float* %x, float* %s_seq) #0 {
entry:
  %N.addr = alloca i32, align 4
  %alpha.addr = alloca float, align 4
  %x.addr = alloca float*, align 8
  %s_seq.addr = alloca float*, align 8
  %i = alloca i32, align 4
  store i32 %N, i32* %N.addr, align 4
  store float %alpha, float* %alpha.addr, align 4
  store float* %x, float** %x.addr, align 8
  store float* %s_seq, float** %s_seq.addr, align 8
  %0 = load float*, float** %x.addr, align 8
  %arrayidx = getelementptr inbounds float, float* %0, i64 0
  %1 = load float, float* %arrayidx, align 4
  %2 = load float*, float** %s_seq.addr, align 8
  %arrayidx1 = getelementptr inbounds float, float* %2, i64 0
  store float %1, float* %arrayidx1, align 4
  store i32 1, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %3 = load i32, i32* %i, align 4
  %4 = load i32, i32* %N.addr, align 4
  %cmp = icmp slt i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %5 = load float, float* %alpha.addr, align 4
  %6 = load float*, float** %x.addr, align 8
  %7 = load i32, i32* %i, align 4
  %idxprom = sext i32 %7 to i64
  %arrayidx2 = getelementptr inbounds float, float* %6, i64 %idxprom
  %8 = load float, float* %arrayidx2, align 4
  %mul = fmul float %5, %8
  %9 = load float, float* %alpha.addr, align 4
  %sub = fsub float 1.000000e+00, %9
  %10 = load float*, float** %s_seq.addr, align 8
  %11 = load i32, i32* %i, align 4
  %sub3 = sub nsw i32 %11, 1
  %idxprom4 = sext i32 %sub3 to i64
  %arrayidx5 = getelementptr inbounds float, float* %10, i64 %idxprom4
  %12 = load float, float* %arrayidx5, align 4
  %mul6 = fmul float %sub, %12
  %add = fadd float %mul, %mul6
  %13 = load float*, float** %s_seq.addr, align 8
  %14 = load i32, i32* %i, align 4
  %idxprom7 = sext i32 %14 to i64
  %arrayidx8 = getelementptr inbounds float, float* %13, i64 %idxprom7
  store float %add, float* %arrayidx8, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %15 = load i32, i32* %i, align 4
  %inc = add nsw i32 %15, 1
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
