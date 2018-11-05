% Generate coefficients for an NxN Gaussian Blur Filter stencil
N = 7;
x=linspace(-1,1,N);
[X,Y]=meshgrid(x);

Ap = 1;  % amplitude
sigma2 = 0.75; % variance

F = Ap*exp(-(X.^2 + Y.^2)/(2*sigma2));

% normalize matrix by its total sum of elements to achieve energy cons
T = sum(sum(F));
Fnorm = F/T;

% ensure sum of all filter coeffs = 1
 filtsum = sum(sum(Fnorm));

% generate comma-separated output string which can be easily used for
% C-style array definition (cut and paste into source code array init)

strFilter = sprintf('%0.4f,',Fnorm)

% plot the 2D Gaussian Surface
figure;
%mesh(X,Y,Fnorm);
surf(X,Y,Fnorm);