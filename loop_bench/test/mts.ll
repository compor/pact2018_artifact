; ModuleID = 'mts.cpp'
source_filename = "mts.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @_Z6serialiiiPi(i32 %N, i32 %s, i32 %p, i32* %A) #0 {
entry:
  %N.addr = alloca i32, align 4
  %s.addr = alloca i32, align 4
  %p.addr = alloca i32, align 4
  %A.addr = alloca i32*, align 8
  %i = alloca i32, align 4
  store i32 %N, i32* %N.addr, align 4
  store i32 %s, i32* %s.addr, align 4
  store i32 %p, i32* %p.addr, align 4
  store i32* %A, i32** %A.addr, align 8
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, i32* %i, align 4
  %1 = load i32, i32* %N.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i32, i32* %s.addr, align 4
  %3 = load i32*, i32** %A.addr, align 8
  %4 = load i32, i32* %i, align 4
  %idxprom = sext i32 %4 to i64
  %arrayidx = getelementptr inbounds i32, i32* %3, i64 %idxprom
  %5 = load i32, i32* %arrayidx, align 4
  %add = add nsw i32 %2, %5
  %cmp1 = icmp sgt i32 %add, 0
  br i1 %cmp1, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  %6 = load i32*, i32** %A.addr, align 8
  %7 = load i32, i32* %i, align 4
  %idxprom2 = sext i32 %7 to i64
  %arrayidx3 = getelementptr inbounds i32, i32* %6, i64 %idxprom2
  %8 = load i32, i32* %arrayidx3, align 4
  %9 = load i32, i32* %s.addr, align 4
  %add4 = add nsw i32 %9, %8
  store i32 %add4, i32* %s.addr, align 4
  br label %if.end

if.else:                                          ; preds = %for.body
  store i32 0, i32* %s.addr, align 4
  %10 = load i32, i32* %i, align 4
  %add5 = add nsw i32 %10, 1
  store i32 %add5, i32* %p.addr, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %11 = load i32, i32* %i, align 4
  %inc = add nsw i32 %11, 1
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
