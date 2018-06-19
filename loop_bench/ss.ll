; ModuleID = 'ss.cpp'
source_filename = "ss.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @_Z6serialiPf(i32 %N, float* %A) #0 {
entry:
  %N.addr = alloca i32, align 4
  %A.addr = alloca float*, align 8
  %m = alloca float, align 4
  %m2 = alloca float, align 4
  %i = alloca i32, align 4
  store i32 %N, i32* %N.addr, align 4
  store float* %A, float** %A.addr, align 8
  store float 9.999990e+05, float* %m, align 4
  store float 9.999990e+05, float* %m2, align 4
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, i32* %i, align 4
  %1 = load i32, i32* %N.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load float, float* %m, align 4
  %3 = load float*, float** %A.addr, align 8
  %4 = load i32, i32* %i, align 4
  %idxprom = sext i32 %4 to i64
  %arrayidx = getelementptr inbounds float, float* %3, i64 %idxprom
  %5 = load float, float* %arrayidx, align 4
  %cmp1 = fcmp ogt float %2, %5
  br i1 %cmp1, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  %6 = load float, float* %m, align 4
  store float %6, float* %m2, align 4
  %7 = load float*, float** %A.addr, align 8
  %8 = load i32, i32* %i, align 4
  %idxprom2 = sext i32 %8 to i64
  %arrayidx3 = getelementptr inbounds float, float* %7, i64 %idxprom2
  %9 = load float, float* %arrayidx3, align 4
  store float %9, float* %m, align 4
  br label %if.end10

if.else:                                          ; preds = %for.body
  %10 = load float*, float** %A.addr, align 8
  %11 = load i32, i32* %i, align 4
  %idxprom4 = sext i32 %11 to i64
  %arrayidx5 = getelementptr inbounds float, float* %10, i64 %idxprom4
  %12 = load float, float* %arrayidx5, align 4
  %13 = load float, float* %m2, align 4
  %cmp6 = fcmp olt float %12, %13
  br i1 %cmp6, label %if.then7, label %if.end

if.then7:                                         ; preds = %if.else
  %14 = load float*, float** %A.addr, align 8
  %15 = load i32, i32* %i, align 4
  %idxprom8 = sext i32 %15 to i64
  %arrayidx9 = getelementptr inbounds float, float* %14, i64 %idxprom8
  %16 = load float, float* %arrayidx9, align 4
  store float %16, float* %m2, align 4
  br label %if.end

if.end:                                           ; preds = %if.then7, %if.else
  br label %if.end10

if.end10:                                         ; preds = %if.end, %if.then
  br label %for.inc

for.inc:                                          ; preds = %if.end10
  %17 = load i32, i32* %i, align 4
  %inc = add nsw i32 %17, 1
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
