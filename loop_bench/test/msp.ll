; ModuleID = 'msp.cpp'
source_filename = "msp.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @_Z6serialfffPfi(float %ma, float %mi, float %m, float* %A, i32 %N) #0 {
entry:
  %ma.addr = alloca float, align 4
  %mi.addr = alloca float, align 4
  %m.addr = alloca float, align 4
  %A.addr = alloca float*, align 8
  %N.addr = alloca i32, align 4
  %i = alloca i32, align 4
  %tmp = alloca float, align 4
  store float %ma, float* %ma.addr, align 4
  store float %mi, float* %mi.addr, align 4
  store float %m, float* %m.addr, align 4
  store float* %A, float** %A.addr, align 8
  store i32 %N, i32* %N.addr, align 4
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, i32* %i, align 4
  %1 = load i32, i32* %N.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load float*, float** %A.addr, align 8
  %3 = load i32, i32* %i, align 4
  %idxprom = sext i32 %3 to i64
  %arrayidx = getelementptr inbounds float, float* %2, i64 %idxprom
  %4 = load float, float* %arrayidx, align 4
  %cmp1 = fcmp oge float %4, 0.000000e+00
  br i1 %cmp1, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  %5 = load float, float* %ma.addr, align 4
  %6 = load float*, float** %A.addr, align 8
  %7 = load i32, i32* %i, align 4
  %idxprom2 = sext i32 %7 to i64
  %arrayidx3 = getelementptr inbounds float, float* %6, i64 %idxprom2
  %8 = load float, float* %arrayidx3, align 4
  %mul = fmul float %5, %8
  %9 = load float*, float** %A.addr, align 8
  %10 = load i32, i32* %i, align 4
  %idxprom4 = sext i32 %10 to i64
  %arrayidx5 = getelementptr inbounds float, float* %9, i64 %idxprom4
  %11 = load float, float* %arrayidx5, align 4
  %cmp6 = fcmp ogt float %mul, %11
  br i1 %cmp6, label %cond.true, label %cond.false

cond.true:                                        ; preds = %if.then
  %12 = load float, float* %ma.addr, align 4
  %13 = load float*, float** %A.addr, align 8
  %14 = load i32, i32* %i, align 4
  %idxprom7 = sext i32 %14 to i64
  %arrayidx8 = getelementptr inbounds float, float* %13, i64 %idxprom7
  %15 = load float, float* %arrayidx8, align 4
  %mul9 = fmul float %12, %15
  br label %cond.end

cond.false:                                       ; preds = %if.then
  %16 = load float*, float** %A.addr, align 8
  %17 = load i32, i32* %i, align 4
  %idxprom10 = sext i32 %17 to i64
  %arrayidx11 = getelementptr inbounds float, float* %16, i64 %idxprom10
  %18 = load float, float* %arrayidx11, align 4
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi float [ %mul9, %cond.true ], [ %18, %cond.false ]
  store float %cond, float* %ma.addr, align 4
  %19 = load float, float* %mi.addr, align 4
  %20 = load float*, float** %A.addr, align 8
  %21 = load i32, i32* %i, align 4
  %idxprom12 = sext i32 %21 to i64
  %arrayidx13 = getelementptr inbounds float, float* %20, i64 %idxprom12
  %22 = load float, float* %arrayidx13, align 4
  %mul14 = fmul float %19, %22
  %23 = load float*, float** %A.addr, align 8
  %24 = load i32, i32* %i, align 4
  %idxprom15 = sext i32 %24 to i64
  %arrayidx16 = getelementptr inbounds float, float* %23, i64 %idxprom15
  %25 = load float, float* %arrayidx16, align 4
  %cmp17 = fcmp olt float %mul14, %25
  br i1 %cmp17, label %cond.true18, label %cond.false22

cond.true18:                                      ; preds = %cond.end
  %26 = load float, float* %mi.addr, align 4
  %27 = load float*, float** %A.addr, align 8
  %28 = load i32, i32* %i, align 4
  %idxprom19 = sext i32 %28 to i64
  %arrayidx20 = getelementptr inbounds float, float* %27, i64 %idxprom19
  %29 = load float, float* %arrayidx20, align 4
  %mul21 = fmul float %26, %29
  br label %cond.end25

cond.false22:                                     ; preds = %cond.end
  %30 = load float*, float** %A.addr, align 8
  %31 = load i32, i32* %i, align 4
  %idxprom23 = sext i32 %31 to i64
  %arrayidx24 = getelementptr inbounds float, float* %30, i64 %idxprom23
  %32 = load float, float* %arrayidx24, align 4
  br label %cond.end25

cond.end25:                                       ; preds = %cond.false22, %cond.true18
  %cond26 = phi float [ %mul21, %cond.true18 ], [ %32, %cond.false22 ]
  store float %cond26, float* %mi.addr, align 4
  br label %if.end

if.else:                                          ; preds = %for.body
  %33 = load float, float* %mi.addr, align 4
  %34 = load float*, float** %A.addr, align 8
  %35 = load i32, i32* %i, align 4
  %idxprom27 = sext i32 %35 to i64
  %arrayidx28 = getelementptr inbounds float, float* %34, i64 %idxprom27
  %36 = load float, float* %arrayidx28, align 4
  %mul29 = fmul float %33, %36
  %37 = load float*, float** %A.addr, align 8
  %38 = load i32, i32* %i, align 4
  %idxprom30 = sext i32 %38 to i64
  %arrayidx31 = getelementptr inbounds float, float* %37, i64 %idxprom30
  %39 = load float, float* %arrayidx31, align 4
  %cmp32 = fcmp ogt float %mul29, %39
  br i1 %cmp32, label %cond.true33, label %cond.false37

cond.true33:                                      ; preds = %if.else
  %40 = load float, float* %mi.addr, align 4
  %41 = load float*, float** %A.addr, align 8
  %42 = load i32, i32* %i, align 4
  %idxprom34 = sext i32 %42 to i64
  %arrayidx35 = getelementptr inbounds float, float* %41, i64 %idxprom34
  %43 = load float, float* %arrayidx35, align 4
  %mul36 = fmul float %40, %43
  br label %cond.end40

cond.false37:                                     ; preds = %if.else
  %44 = load float*, float** %A.addr, align 8
  %45 = load i32, i32* %i, align 4
  %idxprom38 = sext i32 %45 to i64
  %arrayidx39 = getelementptr inbounds float, float* %44, i64 %idxprom38
  %46 = load float, float* %arrayidx39, align 4
  br label %cond.end40

cond.end40:                                       ; preds = %cond.false37, %cond.true33
  %cond41 = phi float [ %mul36, %cond.true33 ], [ %46, %cond.false37 ]
  store float %cond41, float* %tmp, align 4
  %47 = load float, float* %ma.addr, align 4
  %48 = load float*, float** %A.addr, align 8
  %49 = load i32, i32* %i, align 4
  %idxprom42 = sext i32 %49 to i64
  %arrayidx43 = getelementptr inbounds float, float* %48, i64 %idxprom42
  %50 = load float, float* %arrayidx43, align 4
  %mul44 = fmul float %47, %50
  %51 = load float*, float** %A.addr, align 8
  %52 = load i32, i32* %i, align 4
  %idxprom45 = sext i32 %52 to i64
  %arrayidx46 = getelementptr inbounds float, float* %51, i64 %idxprom45
  %53 = load float, float* %arrayidx46, align 4
  %cmp47 = fcmp olt float %mul44, %53
  br i1 %cmp47, label %cond.true48, label %cond.false52

cond.true48:                                      ; preds = %cond.end40
  %54 = load float, float* %ma.addr, align 4
  %55 = load float*, float** %A.addr, align 8
  %56 = load i32, i32* %i, align 4
  %idxprom49 = sext i32 %56 to i64
  %arrayidx50 = getelementptr inbounds float, float* %55, i64 %idxprom49
  %57 = load float, float* %arrayidx50, align 4
  %mul51 = fmul float %54, %57
  br label %cond.end55

cond.false52:                                     ; preds = %cond.end40
  %58 = load float*, float** %A.addr, align 8
  %59 = load i32, i32* %i, align 4
  %idxprom53 = sext i32 %59 to i64
  %arrayidx54 = getelementptr inbounds float, float* %58, i64 %idxprom53
  %60 = load float, float* %arrayidx54, align 4
  br label %cond.end55

cond.end55:                                       ; preds = %cond.false52, %cond.true48
  %cond56 = phi float [ %mul51, %cond.true48 ], [ %60, %cond.false52 ]
  store float %cond56, float* %mi.addr, align 4
  %61 = load float, float* %tmp, align 4
  store float %61, float* %ma.addr, align 4
  br label %if.end

if.end:                                           ; preds = %cond.end55, %cond.end25
  %62 = load float, float* %m.addr, align 4
  %63 = load float, float* %ma.addr, align 4
  %cmp57 = fcmp olt float %62, %63
  br i1 %cmp57, label %if.then58, label %if.end59

if.then58:                                        ; preds = %if.end
  %64 = load float, float* %ma.addr, align 4
  store float %64, float* %m.addr, align 4
  br label %if.end59

if.end59:                                         ; preds = %if.then58, %if.end
  br label %for.inc

for.inc:                                          ; preds = %if.end59
  %65 = load i32, i32* %i, align 4
  %inc = add nsw i32 %65, 1
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
